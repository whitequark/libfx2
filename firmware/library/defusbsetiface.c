#include <fx2usb.h>

void handle_usb_set_interface(uint8_t index, uint8_t value) {
  index;
  if(value == 0) {
    ACK_EP0();
  } else {
    STALL_EP0();
  }
}
