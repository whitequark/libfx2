#include <fx2usb.h>

bool handle_usb_set_configuration(uint8_t value) {
  if(value == 0) {
    SETUP_EP0_ACK();
    return true;
  }
  return false;
}

bool handle_usb_get_configuration() {
  EP0BUF[0] = 0;
  SETUP_EP0_BUF(1);
  return true;
}
