#include <fx2usb.h>

bool handle_usb_clear_endpoint_halt(uint8_t index) {
  index;
  SETUP_EP0_ACK();
  return true;
}
