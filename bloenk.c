// SPDX-License-Identifier: GPL-2.0-only

#include <linux/idr.h>
#include <linux/led-class-multicolor.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/usb.h>

#define VENDOR_ID  0x16c0
#define PRODUCT_ID 0x05dc

#define BLOENK_RQ_SET_CURRENT_LED 0
#define BLOENK_RQ_SET_COLOR_R     1
#define BLOENK_RQ_SET_COLOR_G     2
#define BLOENK_RQ_SET_COLOR_B     3
#define BLOENK_RQ_WRITE_TO_LEDS   4
#define BLOENK_RQ_GET_LEDCOUNT    6

#define BLOENK_LED_SUBLEDS 3
#define BLOENK_LED_MAX_BRIGHTNESS 255

struct bloenk_led {
	struct bloenk_device *bdev;
	struct led_classdev_mc mc_cdev;
	unsigned int brightness[BLOENK_LED_SUBLEDS];
	u8 index;
};
#define lcdev_to_bloenk_led(d) container_of(lcdev_to_mccdev(d), struct bloenk_led, mc_cdev)

struct bloenk_device {
	struct usb_device *usb_dev;
	struct mutex io_mutex;
	int id;
	u8 current_led;
	u8 led_count;
	struct bloenk_led bleds[] __counted_by(led_count);
};

static const struct mc_subled channels[BLOENK_LED_SUBLEDS] = {
	{
		.color_index = LED_COLOR_ID_RED,
		.channel = BLOENK_RQ_SET_COLOR_R,
	},
	{
		.color_index = LED_COLOR_ID_GREEN,
		.channel = BLOENK_RQ_SET_COLOR_G,
	},
	{
		.color_index = LED_COLOR_ID_BLUE,
		.channel = BLOENK_RQ_SET_COLOR_B,
	},
};

DEFINE_IDA(bloenk_ida);

static int bloenk_send_msg(struct usb_device *dev, u8 request, u8 value)
{
	return usb_control_msg(dev, usb_sndctrlpipe(dev, 0), request,
			       USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_DIR_OUT, value, 0, NULL, 0,
			       USB_CTRL_SET_TIMEOUT);
}

static int bloenk_set_brightness(struct led_classdev *cdev, enum led_brightness brightness)
{
	struct bloenk_led *bled;
	struct bloenk_device *bdev;
	struct mc_subled *subled;
	bool changed = false;
	int i, ret;

	bled = lcdev_to_bloenk_led(cdev);
	bdev = bled->bdev;

	mutex_lock(&bdev->io_mutex);

	led_mc_calc_color_components(&bled->mc_cdev, brightness);
	if (bdev->current_led != bled->index) {
		ret = bloenk_send_msg(bdev->usb_dev, BLOENK_RQ_SET_CURRENT_LED, bled->index);
		if (ret < 0)
			goto out;
		bdev->current_led = bled->index;
	}

	for (i = 0; i < bled->mc_cdev.num_colors; ++i) {
		subled = &bled->mc_cdev.subled_info[i];
		if (subled->brightness != bled->brightness[i]) {
			ret = bloenk_send_msg(bdev->usb_dev, subled->channel, subled->brightness);
			if (ret < 0)
				goto out;
			bled->brightness[i] = subled->brightness;
			changed = true;
		}
	}

	if (changed)
		ret = bloenk_send_msg(bdev->usb_dev, BLOENK_RQ_WRITE_TO_LEDS, 0);

out:
	mutex_unlock(&bdev->io_mutex);

	return ret < 0 ? ret : 0;
}

static int bloenk_probe(struct usb_interface *interface, const struct usb_device_id *id)
{
	struct usb_device *usb_dev;
	struct bloenk_device *bdev;
	struct bloenk_led *bled;
	int err = -ENOMEM, retval, i, led_count = 4;

	usb_dev = usb_get_dev(interface_to_usbdev(interface));

	bdev = kzalloc(sizeof(struct bloenk_device) + sizeof(struct bloenk_led) * led_count,
		       GFP_KERNEL);
	if (!bdev)
		return err;

	bdev->led_count = led_count;
	bdev->usb_dev = usb_dev;
	bdev->id = ida_alloc(&bloenk_ida, GFP_KERNEL);
	mutex_init(&bdev->io_mutex);

	for (i = 0; i < led_count; ++i) {
		bled = &bdev->bleds[i];
		bled->bdev = bdev;
		bled->index = i;

		bled->mc_cdev.led_cdev.color = LED_COLOR_ID_MULTI;
		bled->mc_cdev.led_cdev.max_brightness = BLOENK_LED_MAX_BRIGHTNESS;
		bled->mc_cdev.led_cdev.brightness_set_blocking = bloenk_set_brightness;
		bled->mc_cdev.led_cdev.name = kasprintf(GFP_KERNEL, "bloenk%d:%d", bdev->id, i);
		if (!bled->mc_cdev.led_cdev.name)
			goto free_leds;

		bled->mc_cdev.num_colors = BLOENK_LED_SUBLEDS;
		bled->mc_cdev.subled_info = kzalloc(sizeof(channels), GFP_KERNEL);
		if (!bled->mc_cdev.subled_info)
			goto free_name;
		memcpy(bled->mc_cdev.subled_info, channels, sizeof(channels));

		retval = led_classdev_multicolor_register(&interface->dev, &bled->mc_cdev);
		if (retval) {
			err = retval;
			goto free_subled;
		}

		continue;

free_subled:
		kfree(bled->mc_cdev.subled_info);
free_name:
		kfree(bled->mc_cdev.led_cdev.name);
		goto free_leds;
	}
	usb_set_intfdata(interface, bdev);

	return 0;

free_leds:
	for (; i > 0; --i) {
		led_classdev_multicolor_unregister(&bdev->bleds[i - 1].mc_cdev);
		kfree(bdev->bleds[i - 1].mc_cdev.led_cdev.name);
		kfree(bdev->bleds[i - 1].mc_cdev.subled_info);
	}
	ida_free(&bloenk_ida, bdev->id);
	mutex_destroy(&bdev->io_mutex);
	kfree(bdev);
	return err;
}

static void bloenk_disconnect(struct usb_interface *interface)
{
	struct bloenk_device *bdev;
	struct bloenk_led *bled;
	int i;

	bdev = usb_get_intfdata(interface);

	for (i = 0; i < bdev->led_count; ++i) {
		bled = &bdev->bleds[i];
		led_classdev_multicolor_unregister(&bled->mc_cdev);
		kfree(bled->mc_cdev.subled_info);
		kfree(bled->mc_cdev.led_cdev.name);
	}

	ida_free(&bloenk_ida, bdev->id);
	mutex_destroy(&bdev->io_mutex);
	kfree(bdev);
}

static const struct usb_device_id bloenk_table[] = {
	{ USB_DEVICE(VENDOR_ID, PRODUCT_ID) },
	{}
};
MODULE_DEVICE_TABLE(usb, bloenk_table);

static struct usb_driver bloenk_driver = {
	.name = "bloenk",
	.id_table = bloenk_table,
	.probe = bloenk_probe,
	.disconnect = bloenk_disconnect,
};

module_usb_driver(bloenk_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("klMse <git@dnvrqq.com>");
MODULE_DESCRIPTION("USB driver for bloenk");
