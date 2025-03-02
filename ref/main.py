import usb.core
import usb.util
import time
import sys

# Define USB Vendor and Product ID (Replace with actual values if necessary)
VENDOR_ID = 0x16c0  # Example Vendor ID for V-USB
PRODUCT_ID = 0x05dc  # Example Product ID for V-USB

# Custom Request Identifiers (Must match firmware definitions)
CUSTOM_RQ_SET_CURRENT_LED = 0
CUSTOM_RQ_SET_COLOR_R = 1
CUSTOM_RQ_SET_COLOR_G = 2
CUSTOM_RQ_SET_COLOR_B = 3
CUSTOM_RQ_WRITE_TO_LEDS = 4
CUSTOM_RQ_SET_LEDCOUNT = 5
CUSTOM_RQ_GET_LEDCOUNT = 6

# Find USB device
dev = usb.core.find(idVendor=VENDOR_ID, idProduct=PRODUCT_ID)
if dev is None:
    raise ValueError("Device not found")

# Set active configuration
dev.set_configuration()

def set_led(led_index):
    """Set the active LED index."""
    dev.ctrl_transfer(0x40, CUSTOM_RQ_SET_CURRENT_LED, led_index, 0, None)

def set_led_color(r, g, b):
    """Set color of the currently selected LED."""
    dev.ctrl_transfer(0x40, CUSTOM_RQ_SET_COLOR_R, r, 0, None)
    dev.ctrl_transfer(0x40, CUSTOM_RQ_SET_COLOR_G, g, 0, None)
    dev.ctrl_transfer(0x40, CUSTOM_RQ_SET_COLOR_B, b, 0, None)

def write_to_leds():
    """Send the updated colors to the LEDs."""
    try:
        dev.ctrl_transfer(0x40, CUSTOM_RQ_WRITE_TO_LEDS, 0, 0, None)
    except:
        pass

def set_led_count(count):
    """Set the number of LEDs in the strip."""
    dev.ctrl_transfer(0x40, CUSTOM_RQ_SET_LEDCOUNT, count, 0, None)

def get_led_count():
    """Retrieve the current LED count."""
    return 4

def set_led_color_at_index(led_index, r, g, b):
    """Set color of a specific LED by index."""
    set_led(int(led_index))
    set_led_color(int(r), int(g), int(b))
    write_to_leds()

def hsv_to_rgb (h, s, v):
    """Convert HSV to RGB (h: 0-360, s: 0-100, v: 0-100)."""
    h = h / 360.0
    s = s / 100.0
    v = v / 100.0
    if s == 0.0:
        return (v, v, v)
    i = int(h * 6.0)
    f = (h * 6.0) - i
    p = v * (1.0 - s)
    q = v * (1.0 - s * f)
    t = v * (1.0 - s * (1.0 - f))
    i = i % 6
    if i == 0:
        return (v, t, p)
    if i == 1:
        return (q, v, p)
    if i == 2:
        return (p, v, t)
    if i == 3:
        return (p, q, v)
    if i == 4:
        return (t, p, v)
    if i == 5:
        return (v, p, q)

def set_rainbow(angle):
    """Set a rainbow pattern with a specific starting angle."""
    for i in range(get_led_count()):
        r, g, b = hsv_to_rgb((angle + i * 360.0 / get_led_count()) % 360, 100, 100)
        set_led_color_at_index(i, int(r * 255), int(g * 255), int(b * 255))
    write_to_leds()


# Example usage
if __name__ == "__main__":
    while True:
        for angle in range(360):
            set_rainbow(int(angle))


# # Example usage
# if __name__ == "__main__":
#     while True:
#         for i in range(0, 4):
#             set_led(i)
#             set_led_color(255, 0, 0)
#         write_to_leds()
#         time.sleep(0.5)
#         for i in range(0, 4):
#             set_led(i)
#             set_led_color(0,255,0)
#         write_to_leds()
#         time.sleep(0.5)
#         for i in range(0, 4):
#             set_led(i)
#             set_led_color(0, 0, 255)
#         write_to_leds()
#         time.sleep(0.5)
