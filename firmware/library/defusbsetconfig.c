#include <fx2usb.h>

void handle_usb_set_configuration(uint8_t value) {
  if(value == 0 || value == 1) {
    usb_configuration = 0;
    ACK_EP0();
  } else {
    STALL_EP0();
  }
}
