#include <fx2regs.h>
#include <fx2ints.h>
#include <fx2delay.h>
#include <fx2lib.h>
#include <fx2usb.h>

bool usb_self_powered;
bool usb_remote_wakeup;
uint8_t usb_config_value;

void usb_init(bool reconnect) {
  usb_remote_wakeup = false;
  usb_config_value = 0;

  ENABLE_USB_AUTOVEC();
  USBIE  |= _SUDAV;
  USBIRQ  = _SUDAV;
  EA      = 1;

  // Take EP0 under firmware control.
  if(!(USBCS & _RENUM))
    USBCS |= _RENUM;

  // If requested, disconnect and wait for the host to discover that.
  if(reconnect) {
    USBCS |= _DISCON;
    delay_ms(10);
  }

  // Make sure we're connected.
  USBCS &= ~_DISCON;
}

__xdata volatile uint8_t *EPnCS_for_n(uint8_t n) {
  switch(n) {
    case 0x00:
    case 0x80:
      return &EP0CS;
    case 0x01:
      return &EP1OUTCS;
    case 0x81:
      return &EP1INCS;
    case 0x02:
    case 0x82:
      return &EP2CS;
    case 0x04:
    case 0x84:
      return &EP4CS;
    case 0x06:
    case 0x86:
      return &EP6CS;
    case 0x08:
    case 0x88:
      return &EP8CS;
    default:
      return 0;
  }
}

void isr_SUDAV() __interrupt {
  __xdata struct usb_req_setup *req = (__xdata struct usb_req_setup *)SETUPDAT;
  bool handled = false;

  // The sdcc prologue/epilogue only save/restore DPH0/DPL0, but if DPS is 1, then we would
  // in fact modify DPH1/DPL1 when loading dptr with mov dptr.
__asm
  push _DPS
  mov  _DPS, #0
__endasm;

  // Get Descriptor
  if(req->bRequest == USB_REQ_GET_DESCRIPTOR &&
     req->bmRequestType == USB_RECIP_DEVICE|USB_DIR_IN) {
    enum usb_descriptor type = (enum usb_descriptor)(req->wValue >> 8);
    uint8_t index = req->wValue & 0xff;
    handle_usb_get_descriptor(type, index);
    // Set Configuration
  } else if(req->bRequest == USB_REQ_SET_CONFIGURATION &&
            req->bmRequestType == USB_RECIP_DEVICE|USB_DIR_OUT) {
    if(handle_usb_set_configuration((uint8_t)req->wValue)) {
      ACK_EP0();
    } else {
      STALL_EP0();
    }
    // Get Configuration
  } else if(req->bRequest == USB_REQ_GET_CONFIGURATION &&
            req->bmRequestType == USB_RECIP_DEVICE|USB_DIR_IN) {
    handle_usb_get_configuration();
    // Set Interface
  } else if(req->bRequest == USB_REQ_SET_INTERFACE &&
            req->bmRequestType == USB_RECIP_IFACE|USB_DIR_OUT) {
    if(handle_usb_set_interface((uint8_t)req->wIndex, (uint8_t)req->wValue)) {
      ACK_EP0();
    } else {
      STALL_EP0();
    }
    // Get Interface
  } else if(req->bRequest == USB_REQ_GET_INTERFACE &&
            req->bmRequestType == USB_RECIP_IFACE|USB_DIR_IN) {
    handle_usb_get_interface((uint8_t)req->wIndex);
    // Set Feature - Device
  } else if(req->bRequest == USB_REQ_SET_FEATURE &&
            req->bmRequestType == USB_RECIP_DEVICE|USB_DIR_OUT) {
    if(req->wValue == USB_FEAT_DEVICE_REMOTE_WAKEUP) {
      usb_remote_wakeup = true;
      ACK_EP0();
    } else if(req->wValue == USB_FEAT_TEST_MODE) {
      ACK_EP0();
    }
    // Get Status - Device
  } else if(req->bRequest == USB_REQ_GET_STATUS &&
            req->bmRequestType == USB_RECIP_DEVICE|USB_DIR_IN) {
    EP0BUF[0] = (usb_self_powered  << 0) |
                (usb_remote_wakeup << 1);
    EP0BUF[1] = 0;
    SETUP_EP0_BUF(2);
    // Get Status - Interface
  } else if(req->bRequest == USB_REQ_GET_STATUS &&
            req->bmRequestType == USB_RECIP_IFACE|USB_DIR_IN) {
    EP0BUF[0] = 0;
    EP0BUF[1] = 0;
    SETUP_EP0_BUF(2);
    // Set Feature - Endpoint
    // Clear Feature - Endpoint
  } else if(((req->bRequest == USB_REQ_SET_FEATURE ||
              req->bRequest == USB_REQ_CLEAR_FEATURE) &&
             req->bmRequestType == USB_RECIP_ENDPT|USB_DIR_OUT)) {
    if(req->wValue == USB_FEAT_ENDPOINT_HALT) {
      __xdata volatile uint8_t *EPnCS = EPnCS_for_n(req->wIndex);
      if(EPnCS != 0) {
        if(req->bRequest == USB_REQ_SET_FEATURE) {
          *EPnCS |= _STALL;
          ACK_EP0();
        } else {
          if(handle_usb_clear_endpoint_halt((uint8_t)req->wIndex)) {
            *EPnCS &= ~_STALL;
            TOGCTL  = (req->wIndex & 0x0f) | ((req->wIndex & 0x80) >> 3);
            TOGCTL |= _R;
          }
        }
      }
    }
    // Get Status - Endpoint
  } else if(req->bRequest == USB_REQ_GET_STATUS &&
            req->bmRequestType == USB_RECIP_ENDPT|USB_DIR_IN) {
    __xdata volatile uint8_t *EPnCS = EPnCS_for_n(req->wIndex);
    if(EPnCS != 0) {
      EP0BUF[0] = ((*EPnCS & _STALL) != 0);
      EP0BUF[1] = 0;
      SETUP_EP0_BUF(2);
    }
  } else {
    handle_usb_setup(req);
  }

  CLEAR_USB_IRQ();
  USBIRQ = _SUDAV;

__asm
  pop  _DPS
__endasm;
}

static usb_desc_langid_c usb_langid = {
  .bLength          = sizeof(struct usb_desc_langid) + sizeof(uint16_t) * 1,
  .bDescriptorType  = USB_DESC_STRING,
  .wLANGID          = { /* English (United States) */ 0x0409 },
};

void usb_serve_descriptor(usb_descriptor_set_c *set,
                          enum usb_descriptor type, uint8_t index) {
#define APPEND(desc) \
    do { \
      xmemcpy(buf, (__xdata void *)(desc), (desc)->bLength); \
      buf += (desc)->bLength; \
    } while(0)

  __xdata uint8_t *buf = scratch;

  if(type == USB_DESC_DEVICE && index == 0) {
    APPEND(set->device);
  } else if(type == USB_DESC_CONFIGURATION && index < set->config_count) {
    uint8_t nconfig = 0, niface = 0, nendp = 0;
    uint8_t liface, lendp;

    for(nconfig = 0; nconfig < set->config_count; nconfig++) {
      usb_desc_configuration_c *config = &set->configs[nconfig];
      if(nconfig == index)
        APPEND(config);

      for(liface = 0;
          niface < set->interface_count &&
          (liface == 0 || !(set->interfaces[niface].bInterfaceNumber  == 0 &&
                            set->interfaces[niface].bAlternateSetting == 0));
          liface++) {
        usb_desc_interface_c *interface = &set->interfaces[niface++];
        if(nconfig == index)
          APPEND(interface);

        for(lendp = 0; lendp < interface->bNumEndpoints; lendp++) {
          usb_desc_endpoint_c *endpoint = &set->endpoints[nendp++];
          if(nconfig == index)
            APPEND(endpoint);
        }
      }
    }
  } else if(type == USB_DESC_STRING && index == 0) {
    APPEND(&usb_langid);
  } else if(type == USB_DESC_STRING && index - 1 < set->string_count) {
    __code const char *string = set->strings[index - 1];
    *buf++ = 2;               // bLength
    *buf++ = USB_DESC_STRING; // bDescriptorType
    while(*string) {
      *buf++ = *string++;
      *buf++ = 0;
      scratch[0] += 2;
    }
  } else {
    STALL_EP0();
    return;
  }

  SETUP_EP0_IN_DESC(scratch);
}
#undef APPEND

void usb_reset_data_toggles(usb_descriptor_set_c *set, uint8_t interface_num,
                            uint8_t alt_setting) {
  uint8_t nconfig = 0, niface = 0, nendp = 0;
  uint8_t liface, lendp;

  for(nconfig = 0; nconfig < set->config_count; nconfig++) {
    usb_desc_configuration_c *config = &set->configs[nconfig];

    for(liface = 0; liface < config->bNumInterfaces; liface++) {
      usb_desc_interface_c *interface = &set->interfaces[niface++];

      for(lendp = 0; lendp < interface->bNumEndpoints; lendp++) {
        usb_desc_endpoint_c *endpoint = &set->endpoints[nendp++];

        if(config->bConfigurationValue == usb_config_value &&
           (interface_num == 0xff || interface_num == interface->bInterfaceNumber) &&
           (alt_setting == 0xff || alt_setting == interface->bAlternateSetting)) {
          TOGCTL  =  (endpoint->bEndpointAddress & 0x0f) |
                    ((endpoint->bEndpointAddress & 0x80) >> 3);
          TOGCTL |= _R;
        }
      }
    }
  }
}
