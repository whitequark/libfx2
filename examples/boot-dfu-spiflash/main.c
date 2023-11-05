// A DFU bootloader that also supports reading and writing an SPI flash under an alternate
// interface setting.
// See firmware/boot-dfu for an exhaustive description.

#include <fx2lib.h>
#include <fx2delay.h>
#include <fx2eeprom.h>
#include <fx2spiflash.h>
#include <fx2usbdfu.h>

// Memory parameters:
#define EEPROM_SIZE 16384
#define FLASH_SIZE  1048576

// The following routines should handle the multi-master nature of SPI flash usage in
// an application. E.g. if an FPGA is the primary user of the SPI flash, flash_bus_init() should
// also assert FPGA reset, and flash_bus_deinit() would deassert it.
void flash_bus_init(void) {
  OEA |=  0b0111;
}
void flash_bus_deinit(void) {
  OEA &= ~0b0111;
}

DEFINE_SPIFLASH_FNS(flash, /*cs=*/PA0, /*sck=*/PA1, /*si=*/PA2, /*so=*/PA3)

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
  .iSerialNumber        = 0,
  .bNumConfigurations   = 1,
};

extern usb_dfu_desc_functional_c usb_dfu_functional;

usb_desc_interface_c usb_interface_dfu_runtime = {
  .bLength              = sizeof(struct usb_desc_interface),
  .bDescriptorType      = USB_DESC_INTERFACE,
  .bInterfaceNumber     = 0,
  .bAlternateSetting    = 0,
  .bNumEndpoints        = 0,
  .bInterfaceClass      = USB_IFACE_CLASS_APP_SPECIFIC,
  .bInterfaceSubClass   = USB_IFACE_SUBCLASS_DFU,
  .bInterfaceProtocol   = USB_IFACE_PROTOCOL_DFU_RUNTIME,
  .iInterface           = 0,
};

usb_configuration_c usb_config_app = {
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
    { .interface = &usb_interface_dfu_runtime },
    { .generic   = (struct usb_desc_generic *) &usb_dfu_functional },
    { 0 }
  }
};

usb_configuration_set_c usb_configs_app[] = {
  &usb_config_app,
};

usb_ascii_string_c usb_strings_app[] = {
  [0] = "whitequark@whitequark.org",
  [1] = "Example application with DFU support",
};

// DFU mode descriptors

usb_desc_interface_c usb_interface_dfu_upgrade_eeprom = {
  .bLength              = sizeof(struct usb_desc_interface),
  .bDescriptorType      = USB_DESC_INTERFACE,
  .bInterfaceNumber     = 0,
  .bAlternateSetting    = 0,
  .bNumEndpoints        = 0,
  .bInterfaceClass      = USB_IFACE_CLASS_APP_SPECIFIC,
  .bInterfaceSubClass   = USB_IFACE_SUBCLASS_DFU,
  .bInterfaceProtocol   = USB_IFACE_PROTOCOL_DFU_UPGRADE,
  .iInterface           = 3,
};

usb_desc_interface_c usb_interface_dfu_upgrade_flash = {
  .bLength              = sizeof(struct usb_desc_interface),
  .bDescriptorType      = USB_DESC_INTERFACE,
  .bInterfaceNumber     = 0,
  .bAlternateSetting    = 1,
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
    { .interface = &usb_interface_dfu_upgrade_eeprom },
    { .generic   = (struct usb_desc_generic *) &usb_dfu_functional },
    { .interface = &usb_interface_dfu_upgrade_flash },
    { .generic   = (struct usb_desc_generic *) &usb_dfu_functional },
    { 0 }
  }
};

usb_configuration_set_c usb_configs_dfu[] = {
  &usb_config_dfu,
};

usb_ascii_string_c usb_strings_dfu[] = {
  [0] = "whitequark@whitequark.org",
  [1] = "FX2 series DFU-class bootloader",
  [2] = "FX2 boot EEPROM",
  [3] = "Application SPI Flash",
};

// Application and DFU code

__xdata struct usb_descriptor_set usb_descriptor_set = {
  .device           = &usb_device,
  .config_count     = ARRAYSIZE(usb_configs_app),
  .configs          = usb_configs_app,
  .string_count     = ARRAYSIZE(usb_strings_app),
  .strings          = usb_strings_app,
};

usb_dfu_status_t firmware_upload_eeprom(uint32_t address, __xdata uint8_t *data,
                                        __xdata uint16_t *length) {
  if(address < EEPROM_SIZE) {
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

usb_dfu_status_t firmware_dnload_eeprom(uint32_t offset, __xdata uint8_t *data,
                                        uint16_t length) {
  if(length == 0) {
    if(offset == EEPROM_SIZE)
      return USB_DFU_STATUS_OK;
    else
      return USB_DFU_STATUS_errNOTDONE;
  } else if(offset < EEPROM_SIZE) {
    if(eeprom_write(0x51, offset, data, length, /*double_byte=*/true,
                    /*page_size=*/3, /*timeout=*/166)) {
      return USB_DFU_STATUS_OK;
    } else {
      return USB_DFU_STATUS_errWRITE;
    }
  } else {
    return USB_DFU_STATUS_errADDRESS;
  }
}

usb_dfu_status_t firmware_upload_flash(uint32_t offset, __xdata uint8_t *data,
                                       __xdata uint16_t *length) {
  if(offset < FLASH_SIZE) {
    flash_read(offset, data, *length);
    return USB_DFU_STATUS_OK;
  } else {
    *length = 0;
    return USB_DFU_STATUS_OK;
  }
}

usb_dfu_status_t firmware_dnload_flash(uint32_t offset, __xdata uint8_t *data,
                                       uint16_t length) {
  if(length == 0) {
    if(offset == FLASH_SIZE)
      return USB_DFU_STATUS_OK;
    else
      return USB_DFU_STATUS_errNOTDONE;
  } else if(offset >= FLASH_SIZE) {
    return USB_DFU_STATUS_errADDRESS;
  }

  // Because DFU offsets always monotonically increase from 0, we simply erase the entire chip
  // at the very beginning. Depending on the application requirements, this could be adjusted
  // to perform a block erase, incremental sector erase, or something even more complex with
  // multiple boot images in a single flash plus an atomic switch in the dfuMANIFEST phase.
  if(offset == 0) {
    flash_wren();
    flash_ce();
    while(flash_rdsr() & SPIFLASH_WIP);
    if(flash_rdsr() & SPIFLASH_WEL)
      return USB_DFU_STATUS_errERASE;
  }

  // Our DFU block size is fixed at 64 (EP0 buffer size), which means that DFU download requests
  // never cross page boundaries and are smaller than page size of any practical SPI flash.
  // Therefore, we can forward the download requests directly as flash page program requests.
  flash_wren();
  flash_pp(offset, data, length);
  while(flash_rdsr() & SPIFLASH_WIP);
  if(flash_rdsr() & SPIFLASH_WEL)
    return USB_DFU_STATUS_errWRITE;

  return USB_DFU_STATUS_OK;
}

volatile uint8_t dfu_alt_setting = 0;

usb_dfu_status_t firmware_upload(uint32_t offset, __xdata uint8_t *data,
                                 __xdata uint16_t *length) __reentrant {
  if (dfu_alt_setting == 0)
    return firmware_upload_eeprom(offset, data, length);
  if (dfu_alt_setting == 1)
    return firmware_upload_flash(offset, data, length);
  return USB_DFU_STATUS_errUNKNOWN;
}

usb_dfu_status_t firmware_dnload(uint32_t offset, __xdata uint8_t *data,
                                 uint16_t length) __reentrant {
  if (dfu_alt_setting == 0)
    return firmware_dnload_eeprom(offset, data, length);
  if (dfu_alt_setting == 1)
    return firmware_dnload_flash(offset, data, length);
  return USB_DFU_STATUS_errUNKNOWN;
}

usb_dfu_iface_state_t usb_dfu_iface_state = {
  .interface       = 0,
  .firmware_upload = firmware_upload,
  .firmware_dnload = firmware_dnload,
};

void handle_usb_get_interface(uint8_t interface) {
  if(interface == 0) {
    EP0BUF[0] = dfu_alt_setting;
    SETUP_EP0_BUF(1);
    return;
  }
  STALL_EP0();
}

bool handle_usb_set_interface(uint8_t interface, uint8_t alt_setting) {
  if(interface == 0) {
    if(alt_setting == 0 || alt_setting == 1) {
      dfu_alt_setting = alt_setting;
      usb_reset_data_toggles((usb_descriptor_set_c *) &usb_descriptor_set, interface, alt_setting);
      return true;
    }
  }
  return false;
}

void handle_usb_setup(__xdata struct usb_req_setup *req) {
  if(usb_dfu_setup(&usb_dfu_iface_state, req))
    return;

  STALL_EP0();
}

int main(void) {
  CPUCS = _CLKSPD1;

  flash_bus_deinit();

  usb_init(/*disconnect=*/true);

  while(1) {
    if(usb_dfu_iface_state.state == USB_DFU_STATE_appDETACH) {
      delay_ms(10);
      USBCS |= _DISCON;

      usb_dfu_iface_state.state = USB_DFU_STATE_dfuIDLE;

      ((usb_desc_device_t *)usb_device)->idProduct++;
      usb_descriptor_set.config_count = ARRAYSIZE(usb_configs_dfu);
      usb_descriptor_set.configs      = usb_configs_dfu;
      usb_descriptor_set.string_count = ARRAYSIZE(usb_strings_dfu);
      usb_descriptor_set.strings      = usb_strings_dfu;

      flash_bus_init();
      flash_init();
      flash_rdp();

      usb_init(/*disconnect=*/false);
    }

    usb_dfu_setup_deferred(&usb_dfu_iface_state);
  }
}
