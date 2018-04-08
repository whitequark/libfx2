#include <fx2usb.h>

void handle_usb_set_configuration(uint8_t value) {
  if(value == 0) {
    ACK_EP0();
  } else {
    STALL_EP0();
  }
}

void handle_usb_get_configuration() {
  EP0BUF[0] = 0;
  SETUP_EP0_BUF(1);
}
