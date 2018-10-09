#include <fx2usb.h>

bool handle_usb_clear_endpoint_halt(uint8_t endpoint) {
  endpoint;
  ACK_EP0();
  return true;
}
