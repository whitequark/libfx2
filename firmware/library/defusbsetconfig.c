#include <fx2usb.h>

void handle_usb_set_configuration(uint8_t config_value) {
  if(config_value == 0 || config_value == 1) {
    usb_config_value = 0;
    ACK_EP0();
  } else {
    STALL_EP0();
  }
}
