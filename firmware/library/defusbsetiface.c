#include <fx2usb.h>

void handle_usb_set_interface(uint8_t interface, uint8_t alt_setting) {
  interface;
  if(alt_setting == 0) {
    ACK_EP0();
  } else {
    STALL_EP0();
  }
}
