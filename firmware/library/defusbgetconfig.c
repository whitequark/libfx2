#include <fx2usb.h>

void handle_usb_get_configuration() {
  EP0BUF[0] = usb_configuration;
  SETUP_EP0_BUF(1);
}
