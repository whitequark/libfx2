#include <fx2lib.h>
#include <fx2usb.h>
#include <fx2delay.h>
#include <fx2eeprom.h>

usb_desc_device_c usb_device = {
  .bLength              = sizeof(struct usb_desc_device),
  .bDescriptorType      = USB_DESC_DEVICE,
  .bcdUSB               = 0x0200,
  .bDeviceClass         = USB_DEV_CLASS_VENDOR,
  .bDeviceSubClass      = USB_DEV_SUBCLASS_VENDOR,
  .bDeviceProtocol      = USB_DEV_PROTOCOL_VENDOR,
  .bMaxPacketSize0      = 64,
  .idVendor             = 0x04b4,
  .idProduct            = 0x8613,
  .bcdDevice            = 0x0000,
  .iManufacturer        = 1,
  .iProduct             = 2,
  .iSerialNumber        = 0,
  .bNumConfigurations   = 1,
};

usb_desc_interface_c usb_interface = {
  .bLength              = sizeof(struct usb_desc_interface),
  .bDescriptorType      = USB_DESC_INTERFACE,
  .bInterfaceNumber     = 0,
  .bAlternateSetting    = 0,
  .bNumEndpoints        = 0,
  .bInterfaceClass      = USB_IFACE_CLASS_VENDOR,
  .bInterfaceSubClass   = USB_IFACE_SUBCLASS_VENDOR,
  .bInterfaceProtocol   = USB_IFACE_PROTOCOL_VENDOR,
  .iInterface           = 0,
};

usb_configuration_c usb_config = {
  {
    .bLength              = sizeof(struct usb_desc_configuration),
    .bDescriptorType      = USB_DESC_CONFIGURATION,
    .bNumInterfaces       = 1,
    .bConfigurationValue  = 1,
    .iConfiguration       = 0,
    .bmAttributes         = USB_ATTR_RESERVED_1,
    .bMaxPower            = 50,
  },
  {
    { .interface = &usb_interface },
    { 0 }
  }
};

// check for "earlier than 3.5", but version macros shipped in 3.6
#if !defined(__SDCC_VERSION_MAJOR)
__code const struct usb_configuration *__code const usb_configs[] = {
#else
usb_configuration_set_c usb_configs[] = {
#endif
  &usb_config,
};

usb_ascii_string_c usb_strings[] = {
  [0] = "whitequark@whitequark.org",
  [1] = "FX2 series Cypress-class bootloader",
};

usb_descriptor_set_c usb_descriptor_set = {
  .device           = &usb_device,
  .config_count     = ARRAYSIZE(usb_configs),
  .configs          = usb_configs,
  .string_count     = ARRAYSIZE(usb_strings),
  .strings          = usb_strings,
};

enum {
  USB_REQ_CYPRESS_EEPROM_SB  = 0xA2,
  USB_REQ_CYPRESS_EXT_RAM    = 0xA3,
  USB_REQ_CYPRESS_RENUMERATE = 0xA8,
  USB_REQ_CYPRESS_EEPROM_DB  = 0xA9,
  USB_REQ_LIBFX2_PAGE_SIZE   = 0xB0,
};

// We perform lengthy operations in the main loop to avoid hogging the interrupt.
// This flag is used for synchronization between the main loop and the ISR;
// to allow new SETUP requests to arrive while the previous one is still being
// handled (with all data received), the flag should be reset as soon as
// the entire SETUP request is parsed.
volatile bool pending_setup;

void handle_usb_setup(__xdata struct usb_req_setup *req) {
  req;
  if(pending_setup) {
    STALL_EP0();
  } else {
    pending_setup = true;
  }
}

// The EEPROM write cycle time is the same for a single byte or a single page;
// it is therefore far more efficient to write EEPROMs in entire pages.
// Unfortunately, there is no way to discover page size if it is not known
// beforehand. We play it safe and write individual bytes unless the page size
// was set explicitly via a libfx2-specific request, such that Cypress vendor
// requests A2/A9 work the same as in Cypress libraries by default.
uint8_t page_size = 0; // log2(page size in bytes)

void handle_pending_usb_setup(void) {
  __xdata struct usb_req_setup *req = (__xdata struct usb_req_setup *)SETUPDAT;

  if(req->bmRequestType == (USB_RECIP_DEVICE|USB_TYPE_VENDOR|USB_DIR_OUT) &&
     req->bRequest == USB_REQ_CYPRESS_RENUMERATE) {
    pending_setup = false;

    USBCS |= _DISCON;
    delay_ms(10);
    USBCS &= ~_DISCON;

    return;
  }

  if(req->bmRequestType == (USB_RECIP_DEVICE|USB_TYPE_VENDOR|USB_DIR_OUT) &&
     req->bRequest == USB_REQ_LIBFX2_PAGE_SIZE) {
    page_size = req->wValue;
    pending_setup = false;

    ACK_EP0();
    return;
  }

  if((req->bmRequestType == (USB_RECIP_DEVICE|USB_TYPE_VENDOR|USB_DIR_IN) ||
      req->bmRequestType == (USB_RECIP_DEVICE|USB_TYPE_VENDOR|USB_DIR_OUT)) &&
     (req->bRequest == USB_REQ_CYPRESS_EEPROM_SB ||
      req->bRequest == USB_REQ_CYPRESS_EEPROM_DB)) {
    bool     arg_read  = (req->bmRequestType & USB_DIR_IN);
    bool     arg_dbyte = (req->bRequest == USB_REQ_CYPRESS_EEPROM_DB);
    uint8_t  arg_chip  = arg_dbyte ? 0x51 : 0x50;
    uint16_t arg_addr  = req->wValue;
    uint16_t arg_len   = req->wLength;
    pending_setup = false;

    while(arg_len > 0) {
      uint8_t len = arg_len < 64 ? arg_len : 64;

      if(arg_read) {
        while(EP0CS & _BUSY);
        if(!eeprom_read(arg_chip, arg_addr, EP0BUF, len, arg_dbyte)) {
          STALL_EP0();
          break;
        }
        SETUP_EP0_IN_BUF(len);
      } else {
        SETUP_EP0_OUT_BUF();
        while(EP0CS & _BUSY);
        if(!eeprom_write(arg_chip, arg_addr, EP0BUF, len, arg_dbyte, page_size,
                         /*timeout=*/166)) {
          STALL_EP0();
          break;
        }
        ACK_EP0();
      }

      arg_len  -= len;
      arg_addr += len;
    }

    return;
  }

  if((req->bmRequestType == (USB_RECIP_DEVICE|USB_TYPE_VENDOR|USB_DIR_IN) ||
      req->bmRequestType == (USB_RECIP_DEVICE|USB_TYPE_VENDOR|USB_DIR_OUT)) &&
     req->bRequest == USB_REQ_CYPRESS_EXT_RAM) {
    bool     arg_read = (req->bmRequestType & USB_DIR_IN);
    uint16_t arg_addr = req->wValue;
    uint16_t arg_len  = req->wLength;
    pending_setup = false;

    while(arg_len > 0) {
      uint8_t len = arg_len < 64 ? arg_len : 64;

      if(arg_read) {
        while(EP0CS & _BUSY);
        xmemcpy(EP0BUF, (__xdata void *)arg_addr, len);
        SETUP_EP0_IN_BUF(len);
      } else {
        SETUP_EP0_OUT_BUF();
        while(EP0CS & _BUSY);
        xmemcpy((__xdata void *)arg_addr, EP0BUF, arg_len);
        ACK_EP0();
      }

      arg_len  -= len;
      arg_addr += len;
    }

    return;
  }

  pending_setup = false;
  STALL_EP0();
}

int main(void) {
  CPUCS = _CLKOE|_CLKSPD1;

  // Don't re-enumerate. `fx2tool -B` will load this firmware to access EEPROM, and it
  // expects to be able to keep accessing the device. If you are using this firmware
  // in your own code, set /*diconnect=*/true.
  usb_init(/*disconnect=*/false);

  while(1) {
    if(pending_setup)
      handle_pending_usb_setup();
  }
}
