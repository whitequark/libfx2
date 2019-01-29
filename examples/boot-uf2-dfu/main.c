// A combined U2F and DFU bootloader, for maximum compatibility.
// See firmware/boot-dfu and firmware/boot-uf2 for exhaustive descriptions of either mode.

#include <fx2lib.h>
#include <fx2delay.h>
#include <fx2eeprom.h>
#include <fx2usbdfu.h>
#include <fx2usbmassstor.h>
#include <fx2uf2.h>

// Memory parameters:
#define FIRMWARE_SIZE 16384

// Application mode descriptors.

usb_desc_device_c usb_device = {
  .bLength              = sizeof(struct usb_desc_device),
  .bDescriptorType      = USB_DESC_DEVICE,
  .bcdUSB               = 0x0200,
  .bDeviceClass         = USB_DEV_CLASS_PER_INTERFACE,
  .bDeviceSubClass      = USB_DEV_SUBCLASS_PER_INTERFACE,
  .bDeviceProtocol      = USB_DEV_PROTOCOL_PER_INTERFACE,
  .bMaxPacketSize0      = 64,
  .idVendor             = 0x04b4,
  .idProduct            = 0x8613,
  .bcdDevice            = 0x0000,
  .iManufacturer        = 1,
  .iProduct             = 2,
  .iSerialNumber        = 3,
  .bNumConfigurations   = 1,
};

usb_desc_interface_c usb_interface_mass_storage = {
  .bLength              = sizeof(struct usb_desc_interface),
  .bDescriptorType      = USB_DESC_INTERFACE,
  .bInterfaceNumber     = 0,
  .bAlternateSetting    = 0,
  .bNumEndpoints        = 2,
  .bInterfaceClass      = USB_IFACE_CLASS_MASS_STORAGE,
  .bInterfaceSubClass   = USB_IFACE_SUBCLASS_MASS_STORAGE_SCSI,
  .bInterfaceProtocol   = USB_IFACE_PROTOCOL_MASS_STORAGE_BBB,
  .iInterface           = 0,
};

usb_desc_endpoint_c usb_endpoint_ep2_out = {
  .bLength              = sizeof(struct usb_desc_endpoint),
  .bDescriptorType      = USB_DESC_ENDPOINT,
  .bEndpointAddress     = 2,
  .bmAttributes         = USB_XFER_BULK,
  .wMaxPacketSize       = 512,
  .bInterval            = 0,
};

usb_desc_endpoint_c usb_endpoint_ep6_in = {
  .bLength              = sizeof(struct usb_desc_endpoint),
  .bDescriptorType      = USB_DESC_ENDPOINT,
  .bEndpointAddress     = 6|USB_DIR_IN,
  .bmAttributes         = USB_XFER_BULK,
  .wMaxPacketSize       = 512,
  .bInterval            = 0,
};

usb_desc_interface_c usb_interface_dfu_runtime = {
  .bLength              = sizeof(struct usb_desc_interface),
  .bDescriptorType      = USB_DESC_INTERFACE,
  .bInterfaceNumber     = 1,
  .bAlternateSetting    = 0,
  .bNumEndpoints        = 0,
  .bInterfaceClass      = USB_IFACE_CLASS_APP_SPECIFIC,
  .bInterfaceSubClass   = USB_IFACE_SUBCLASS_DFU,
  .bInterfaceProtocol   = USB_IFACE_PROTOCOL_DFU_RUNTIME,
  .iInterface           = 0,
};

usb_configuration_c usb_config_uf2 = {
  {
    .bLength              = sizeof(struct usb_desc_configuration),
    .bDescriptorType      = USB_DESC_CONFIGURATION,
    .bNumInterfaces       = 2,
    .bConfigurationValue  = 1,
    .iConfiguration       = 0,
    .bmAttributes         = USB_ATTR_RESERVED_1,
    .bMaxPower            = 50,
  },
  {
    { .interface  = &usb_interface_mass_storage },
    { .endpoint   = &usb_endpoint_ep2_out },
    { .endpoint   = &usb_endpoint_ep6_in  },
    { .interface  = &usb_interface_dfu_runtime },
    { .generic    = (struct usb_desc_generic *) &usb_dfu_functional },
    { 0 }
  }
};

usb_configuration_set_c usb_configs_uf2[] = {
  &usb_config_uf2,
};

usb_ascii_string_c usb_strings_uf2[] = {
  [0] = "whitequark@whitequark.org",
  [1] = "FX2 series DFU/UF2-class bootloader, UF2 mode",
  [2] = "000000000000",
};

// DFU mode descriptors

usb_desc_interface_c usb_interface_dfu_upgrade = {
  .bLength              = sizeof(struct usb_desc_interface),
  .bDescriptorType      = USB_DESC_INTERFACE,
  .bInterfaceNumber     = 0,
  .bAlternateSetting    = 0,
  .bNumEndpoints        = 0,
  .bInterfaceClass      = USB_IFACE_CLASS_APP_SPECIFIC,
  .bInterfaceSubClass   = USB_IFACE_SUBCLASS_DFU,
  .bInterfaceProtocol   = USB_IFACE_PROTOCOL_DFU_UPGRADE,
  .iInterface           = 4,
};

usb_dfu_desc_functional_c usb_dfu_functional = {
  .bLength              = sizeof(struct usb_dfu_desc_functional),
  .bDescriptorType      = USB_DESC_DFU_FUNCTIONAL,
  .bmAttributes         = USB_DFU_ATTR_CAN_DNLOAD |
                          USB_DFU_ATTR_CAN_UPLOAD |
                          USB_DFU_ATTR_MANIFESTATION_TOLERANT |
                          USB_DFU_ATTR_WILL_DETACH,
  .wTransferSize        = 64,
  .bcdDFUVersion        = 0x0101,
};

usb_configuration_c usb_config_dfu = {
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
    { .interface = &usb_interface_dfu_upgrade },
    { .generic   = (struct usb_desc_generic *) &usb_dfu_functional },
    { 0 }
  }
};

usb_configuration_set_c usb_configs_dfu[] = {
  &usb_config_dfu,
};

usb_ascii_string_c usb_strings_dfu[] = {
  [0] = "whitequark@whitequark.org",
  [1] = "FX2 series DFU/UF2-class bootloader, DFU mode",
  [2] = "000000000000",
  [3] = "Boot EEPROM"
};

// Common UF2/DFU code

__xdata struct usb_descriptor_set usb_descriptor_set = {
  .device           = &usb_device,
  .config_count     = ARRAYSIZE(usb_configs_uf2),
  .configs          = usb_configs_uf2,
  .string_count     = ARRAYSIZE(usb_strings_uf2),
  .strings          = usb_strings_uf2,
};

// UF2 support code

usb_mass_storage_bbb_state_t usb_mass_storage_state = {
  .interface    = 0,
  .max_in_size  = 512,

  .command      = uf2_scsi_command,
  .data_out     = uf2_scsi_data_out,
  .data_in      = uf2_scsi_data_in,
};

static bool firmware_read(uint32_t address, __xdata uint8_t *data, uint16_t length) __reentrant {
  return eeprom_read(0x51, address, data, length, /*double_byte=*/true);
}

static bool firmware_write(uint32_t address, __xdata uint8_t *data, uint16_t length) __reentrant {
  return eeprom_write(0x51, address, data, length, /*double_byte=*/true,
                      /*page_size=*/3, /*timeout=*/166);
}

uf2_configuration_c uf2_config = {
  .total_sectors  = 2 * 32768,
  .info_uf2_txt   =
    "DFU/UF2 Bootloader for Cypress FX2\r\n"
    "Model: Generic Developer Board with 16Kx8 EEPROM\r\n"
    "Board-ID: FX2-Generic_16Kx8-v0\r\n",
  .index_htm      =
    "<meta http-equiv=\"refresh\" content=\"0; url=https://github.com/whitequark/libfx2/\">",
  .firmware_size  = FIRMWARE_SIZE,

  .firmware_read  = firmware_read,
  .firmware_write = firmware_write,
};

// DFU support code

usb_dfu_status_t firmware_upload(uint32_t address, __xdata uint8_t *data,
                                 __xdata uint16_t *length) __reentrant {
  if(address < FIRMWARE_SIZE) {
    // Only 2-byte EEPROMs are large enough to store any sort of firmware, and the address
    // of a 2-byte boot EEPROM is fixed, so it's safe to hardcode it here.
    if(eeprom_read(0x51, address, data, *length, /*double_byte=*/true)) {
      return USB_DFU_STATUS_OK;
    } else {
      return USB_DFU_STATUS_errUNKNOWN;
    }
  } else {
    *length = 0;
    return USB_DFU_STATUS_OK;
  }
}

usb_dfu_status_t firmware_dnload(uint32_t address, __xdata uint8_t *data,
                                 uint16_t length) __reentrant {
  if(length == 0) {
    if(address == FIRMWARE_SIZE)
      return USB_DFU_STATUS_OK;
    else
      return USB_DFU_STATUS_errNOTDONE;
  } else if(address < FIRMWARE_SIZE) {
    if(eeprom_write(0x51, address, data, length, /*double_byte=*/true,
                    /*page_size=*/3, /*timeout=*/166)) {
      return USB_DFU_STATUS_OK;
    } else {
      return USB_DFU_STATUS_errWRITE;
    }
  } else {
    return USB_DFU_STATUS_errADDRESS;
  }
}

usb_dfu_iface_state_t usb_dfu_iface_state = {
  .interface          = 1,

  .firmware_upload    = firmware_upload,
  .firmware_dnload    = firmware_dnload,
};

// Initialization and common request handling code

void handle_usb_setup(__xdata struct usb_req_setup *req) {
  if(usb_dfu_iface_state.state == USB_DFU_STATE_appIDLE) {
    if(usb_mass_storage_bbb_setup(&usb_mass_storage_state, req))
      return;
  }

  if(usb_dfu_setup(&usb_dfu_iface_state, req))
    return;

  STALL_EP0();
}

volatile bool pending_ep6_in;

void isr_IBN() __interrupt {
  pending_ep6_in = true;
  CLEAR_USB_IRQ();
  NAKIRQ = _IBN;
  IBNIRQ = _IBNI_EP6;
}

int main() {
  CPUCS = _CLKSPD1;

  REVCTL = _ENH_PKT|_DYN_OUT;

  SYNCDELAY;
  FIFORESET = _NAKALL;

  EP2CFG  =  _VALID|_TYPE1|_BUF1;
  EP2CS   = 0;
  EP6CFG  =  _VALID|_DIR|_TYPE1|_BUF1;
  EP6CS   = 0;
  EP4CFG &= ~_VALID;
  EP8CFG &= ~_VALID;

  IBNIE = _IBNI_EP6;
  NAKIE = _IBN;

  SYNCDELAY;
  FIFORESET = _NAKALL|2;
  SYNCDELAY;
  OUTPKTEND = _SKIP|2;
  SYNCDELAY;
  OUTPKTEND = _SKIP|2;
  SYNCDELAY;
  FIFORESET = _NAKALL|6;
  SYNCDELAY;
  FIFORESET = 0;

  usb_init(/*disconnect=*/true);

  while(1) {
    // Handle pending UF2 requests.

    if(!(EP2CS & _EMPTY)) {
      uint16_t length = (EP2BCH << 8) | EP2BCL;
      if(usb_mass_storage_bbb_bulk_out(&usb_mass_storage_state, EP2FIFOBUF, length)) {
        EP2BCL = 0;
      } else {
        EP2CS  = _STALL;
        EP6CS  = _STALL;
      }
    }

    if(pending_ep6_in) {
      __xdata uint16_t length;
      if(usb_mass_storage_bbb_bulk_in(&usb_mass_storage_state, EP6FIFOBUF, &length)) {
        if(length > 0) {
          EP6BCH = length >> 8;
          SYNCDELAY;
          EP6BCL = length;
        }
      } else {
        EP6CS  = _STALL;
      }

      pending_ep6_in = false;
    }

    // Handle pending DFU requests.

    if(usb_dfu_iface_state.state == USB_DFU_STATE_appDETACH) {
      delay_ms(10);
      USBCS |= _DISCON;

      usb_dfu_iface_state.state = USB_DFU_STATE_dfuIDLE;

      ((usb_desc_device_t *)usb_device)->idProduct++;
      usb_descriptor_set.config_count = ARRAYSIZE(usb_configs_dfu);
      usb_descriptor_set.configs      = usb_configs_dfu;
      usb_descriptor_set.string_count = ARRAYSIZE(usb_strings_dfu);
      usb_descriptor_set.strings      = usb_strings_dfu;

      usb_init(/*disconnect=*/false);
    }

    usb_dfu_setup_deferred(&usb_dfu_iface_state);
  }
}
