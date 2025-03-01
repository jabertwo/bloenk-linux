// #include <linux/init.h>
// #include <linux/kernel.h>
// #include <linux/module.h>

// static int __init bloenk_init(void)
// {
// 	return 0;
// }

// static void __exit bloenk_exit(void)
// {

// }

// module_init(bloenk_init);
// module_exit(bloenk_exit);

// MODULE_DESCRIPTION("bloenk USB driver");
// MODULE_AUTHOR("klMse");
// MODULE_LICENSE("GPL");

/*
 * usbled.c - A Linux kernel driver to control a custom USB LED device
 * via /sys/class/leds.
 *
 * This driver registers three LED class devices (red, green, blue).
 * Each brightness change sends a USB vendor request to:
 *   - Set the current LED (index 0),
 *   - Set the corresponding channel value, and then
 *   - Write the LED update to the device.
 *
 * The vendor-specific request codes must match those in the firmware.
 */

 #include <linux/module.h>
 #include <linux/usb.h>
 #include <linux/leds.h>
 #include <linux/mutex.h>
 #include <linux/slab.h>
 
 #define VENDOR_ID  0x16c0  // Replace with your device's vendor ID
 #define PRODUCT_ID 0x05dc  // Replace with your device's product ID
 
 #define CUSTOM_RQ_SET_CURRENT_LED  0
 #define CUSTOM_RQ_SET_COLOR_R      1
 #define CUSTOM_RQ_SET_COLOR_G      2
 #define CUSTOM_RQ_SET_COLOR_B      3
 #define CUSTOM_RQ_WRITE_TO_LEDS    4
 #define CUSTOM_RQ_GET_LEDCOUNT     6
 
 static struct usb_device *usb_led_dev;
 static struct led_classdev **usb_leds;
 static struct mutex usb_led_mutex;
 static int led_count = 0;
 
 static int usb_led_control(struct usb_device *dev, u8 request, u8 value, u8 index)
 {
	 return usb_control_msg(dev, usb_sndctrlpipe(dev, 0), request,
							USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_DIR_OUT,
							value, index, NULL, 0, USB_CTRL_SET_TIMEOUT);
 }
 
 static int usb_led_get_count(struct usb_device *dev)
 {
	return 4;
 }
 
 static void usb_led_set_brightness(struct led_classdev *led_cdev,
									enum led_brightness brightness)
 {
	 int index = (int)(uintptr_t)led_cdev->dev->platform_data;
	 mutex_lock(&usb_led_mutex);
	 usb_led_control(usb_led_dev, CUSTOM_RQ_SET_CURRENT_LED, index, 0);
	 usb_led_control(usb_led_dev, CUSTOM_RQ_SET_COLOR_R, brightness, 0);
	 usb_led_control(usb_led_dev, CUSTOM_RQ_WRITE_TO_LEDS, 0, 0);
	 mutex_unlock(&usb_led_mutex);
 }
 
 static int usb_led_probe(struct usb_interface *interface, const struct usb_device_id *id)
 {
	 int i, ret;
	 usb_led_dev = usb_get_dev(interface_to_usbdev(interface));
	 mutex_init(&usb_led_mutex);
	 
	 led_count = usb_led_get_count(usb_led_dev);
	 if (led_count <= 0) {
		 dev_err(&interface->dev, "Failed to get LED count\n");
		 return -ENODEV;
	 }
	 
	 usb_leds = kcalloc(led_count, sizeof(struct led_classdev *), GFP_KERNEL);
	 if (!usb_leds)
		 return -ENOMEM;
	 
	 for (i = 0; i < led_count; i++) {
		 usb_leds[i] = kzalloc(sizeof(struct led_classdev), GFP_KERNEL);
		 if (!usb_leds[i]) {
			 ret = -ENOMEM;
			 goto error;
		 }
		 usb_leds[i]->name = kasprintf(GFP_KERNEL, "usb_led_%d", i);
		 usb_leds[i]->brightness_set = usb_led_set_brightness;
		 usb_leds[i]->max_brightness = 255;
		 usb_leds[i]->dev->platform_data = (void *)(uintptr_t)i;
		 led_classdev_register(&interface->dev, usb_leds[i]);
	 }
	 
	 return 0;
 
 error:
	 while (i-- > 0) {
		 led_classdev_unregister(usb_leds[i]);
		 kfree(usb_leds[i]->name);
		 kfree(usb_leds[i]);
	 }
	 kfree(usb_leds);
	 return ret;
 }
 
 static void usb_led_disconnect(struct usb_interface *interface)
 {
	 int i;
	 for (i = 0; i < led_count; i++) {
		 led_classdev_unregister(usb_leds[i]);
		 kfree(usb_leds[i]->name);
		 kfree(usb_leds[i]);
	 }
	 kfree(usb_leds);
	 usb_put_dev(usb_led_dev);
 }
 
 static struct usb_device_id usb_led_table[] = {
	 { USB_DEVICE(VENDOR_ID, PRODUCT_ID) },
	 {}
 };
 MODULE_DEVICE_TABLE(usb, usb_led_table);
 
 static struct usb_driver usb_led_driver = {
	 .name = "usb_led",
	 .id_table = usb_led_table,
	 .probe = usb_led_probe,
	 .disconnect = usb_led_disconnect,
 };
 
 module_usb_driver(usb_led_driver);
 
 MODULE_LICENSE("GPL");
 MODULE_AUTHOR("Your Name");
 MODULE_DESCRIPTION("USB RGB LED Driver for /sys/class/leds with per-LED control"); 