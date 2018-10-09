#include <fx2usb.h>

extern usb_descriptor_set_c usb_descriptor_set;

bool handle_usb_set_configuration(uint8_t config_value) {
  if(config_value == 0 || config_value == 1) {
    usb_config_value = config_value;

    usb_reset_data_toggles(&usb_descriptor_set, /*inteface=*/0xff, /*alt_setting=*/0xff);
    return true;
  }

  return false;
}
