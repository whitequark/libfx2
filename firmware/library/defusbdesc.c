#include <fx2usb.h>

extern usb_descriptor_set_c usb_descriptor_set;

void handle_usb_get_descriptor(enum usb_descriptor type, uint8_t index) {
  usb_serve_descriptor(&usb_descriptor_set, type, index);
}
