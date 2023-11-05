#include <fx2lib.h>
#include <fx2delay.h>
#include <fx2eeprom.h>
#include <fx2usbdfu.h>

// Replace this with the actual EEPROM size on your board to use its full capacity.
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

usb_desc_interface_c usb_interface_dfu_upgrade = {
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
  [1] = "FX2 series DFU-class bootloader",
  [2] = "Boot EEPROM"
};

// Application and DFU code

__xdata struct usb_descriptor_set usb_descriptor_set = {
  .device           = &usb_device,
  .config_count     = ARRAYSIZE(usb_configs_app),
  .configs          = usb_configs_app,
  .string_count     = ARRAYSIZE(usb_strings_app),
  .strings          = usb_strings_app,
};

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
    // Use 8-byte page writes, which are slow but universally compatible. (Strictly speaking,
    // no EEPROM can be assumed to provide any page writes, but virtually every EEPROM larger
    // than 16 KiB supports at least 8-byte pages).
    //
    // If the datasheet for the EEPROM lists larger pages as permissible, this would provide
    // a significant speed boost. Unfortunately it is not really possible to discover the page
    // size by interrogating the EEPROM.
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

usb_dfu_status_t firmware_manifest(void) __reentrant {
  // Simulate committing the firmware. If this function is not necessary, it may simply be omitted,
  // together with its entry in `usb_dfu_iface_state`.
  delay_ms(1000);

  return USB_DFU_STATUS_OK;
}

usb_dfu_iface_state_t usb_dfu_iface_state = {
  // Set to bInterfaceNumber of the DFU descriptor in the application mode.
  .interface          = 0,

  .firmware_upload    = firmware_upload,
  .firmware_dnload    = firmware_dnload,
  .firmware_manifest  = firmware_manifest,
};

void handle_usb_setup(__xdata struct usb_req_setup *req) {
  if(usb_dfu_setup(&usb_dfu_iface_state, req))
    return;

  STALL_EP0();
}

int main(void) {
  // Run core at 48 MHz fCLK.
  CPUCS = _CLKSPD1;

  // Re-enumerate, to make sure our descriptors are picked up correctly.
  usb_init(/*disconnect=*/true);

  while(1) {
    // Handle switching to DFU mode from application mode.
    if(usb_dfu_iface_state.state == USB_DFU_STATE_appDETACH) {
      // Wait until the host has received our ACK of the DETACH request before actually
      // disconnecting. This is because if we disconnect immediately, the host might just
      // return an error to the DFU utility.
      delay_ms(10);
      USBCS |= _DISCON;

      // Switch to DFU mode.
      usb_dfu_iface_state.state = USB_DFU_STATE_dfuIDLE;

      // Re-enumerate using the DFU mode descriptors. For Windows compatibility, it is necessary
      // to change USB Product ID in the Device descriptor as well, since Windows is unable
      // to rebind a DFU driver to the same VID:PID pair. (Windows is euphemistically called out
      // in the DFU spec as a "certain operating system").
      ((usb_desc_device_t *)usb_device)->idProduct++;
      usb_descriptor_set.config_count = ARRAYSIZE(usb_configs_dfu);
      usb_descriptor_set.configs      = usb_configs_dfu;
      usb_descriptor_set.string_count = ARRAYSIZE(usb_strings_dfu);
      usb_descriptor_set.strings      = usb_strings_dfu;

      // Don't reconnect again in `usb_init`, as we have just disconnected explicitly.
      usb_init(/*disconnect=*/false);
    }

    // Handle any lengthy DFU requests, i.e. the ones that call back into firmware_* functions.
    usb_dfu_setup_deferred(&usb_dfu_iface_state);
  }
}
