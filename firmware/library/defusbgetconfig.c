#include <fx2usb.h>

void handle_usb_get_configuration(void) {
  EP0BUF[0] = usb_config_value;
  SETUP_EP0_IN_BUF(1);
}
