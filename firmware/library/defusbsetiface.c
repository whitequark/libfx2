#include <fx2usb.h>

extern usb_descriptor_set_c usb_descriptor_set;

bool handle_usb_set_interface(uint8_t interface, uint8_t alt_setting) {
  interface;
  if(alt_setting == 0) {
    usb_reset_data_toggles(&usb_descriptor_set, interface, alt_setting);
    return true;
  }

  return false;
}
