#include <fx2lib.h>
#include <fx2delay.h>
#include <fx2eeprom.h>
#include <fx2usbmassstor.h>
#include <fx2uf2.h>

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
    { .interface  = &usb_interface_mass_storage },
    { .endpoint   = &usb_endpoint_ep2_out },
    { .endpoint   = &usb_endpoint_ep6_in  },
    { 0 }
  }
};

usb_configuration_set_c usb_configs[] = {
  &usb_config,
};

usb_ascii_string_c usb_strings[] = {
  [0] = "whitequark@whitequark.org",
  [1] = "FX2 series UF2-class bootloader",
  // USB MS BBB 4.1.1 requires each device to have an unique serial number that is at least
  // 12 characters long. We cannot satisfy the uniqueness requirement, but we at least provide
  // a serial number in a valid format.
  [2] = "000000000000",
};

usb_descriptor_set_c usb_descriptor_set = {
  .device          = &usb_device,
  .config_count    = ARRAYSIZE(usb_configs),
  .configs         = usb_configs,
  .string_count    = ARRAYSIZE(usb_strings),
  .strings         = usb_strings,
};

usb_mass_storage_bbb_state_t usb_mass_storage_state = {
  .interface    = 0,
  .max_in_size  = 512,

  .command      = uf2_scsi_command,
  .data_out     = uf2_scsi_data_out,
  .data_in      = uf2_scsi_data_in,
};

static bool firmware_read(uint32_t address, __xdata uint8_t *data, uint16_t length) __reentrant {
  // Only 2-byte EEPROMs are large enough to store any sort of firmware, and the address
  // of a 2-byte boot EEPROM is fixed, so it's safe to hardcode it here.
  return eeprom_read(0x51, address, data, length, /*double_byte=*/true);
}

static bool firmware_write(uint32_t address, __xdata uint8_t *data, uint16_t length) __reentrant {
  // Use 8-byte page writes, which are slow but universally compatible. (Strictly speaking,
  // no EEPROM can be assumed to provide any page writes, but virtually every EEPROM larger
  // than 16 KiB supports at least 8-byte pages).
  //
  // If the datasheet for the EEPROM lists larger pages as permissible, this would provide
  // a significant speed boost. Unfortunately it is not really possible to discover the page
  // size by interrogating the EEPROM.
  return eeprom_write(0x51, address, data, length, /*double_byte=*/true,
                      /*page_size=*/3, /*timeout=*/166);
}

// Configure for 16Kx8 EEPROM, since this is upwards compatible with larger EEPROMs and
// any application integrating the UF2 bootloader will be at least ~12 KB in size.
// (The overhead of the bootloader is smaller than that, since much of the USB machinery
// can be shared between the bootloader and the application.)
uf2_configuration_c uf2_config = {
  // Provide a virtual mass storage device of 32 MiB in size. Using a device that is
  // too small will result in broken filesystem being generated (in particular, below
  // a certain cluster count, the filesystm gets interpreted as FAT12 instead of FAT16),
  // and a device that is too large will result in slower operations (mounting, etc).
  // 32 MiB is a good number.
  .total_sectors  = 2 * 32768,
  // Replace the Model: and Board-ID: fields with ones specific for your board.
  // Note that Board-ID: field should be machine-readable.
  // The INFO_UF2.TXT file can be up to 512 bytes in size.
  .info_uf2_txt   =
    "UF2 Bootloader for Cypress FX2\r\n"
    "Model: Generic Developer Board with 16Kx8 EEPROM\r\n"
    "Board-ID: FX2-Generic_16Kx8-v0\r\n",
  // Replace the URL with a hyperlink to a document describing your board.
  .index_htm      =
    "<meta http-equiv=\"refresh\" content=\"0; url=https://github.com/whitequark/libfx2/\">",
  // Replace this with the actual EEPROM size on your board to use its full capacity.
  .firmware_size  = 16384,

  .firmware_read  = firmware_read,
  .firmware_write = firmware_write,
};

void handle_usb_setup(__xdata struct usb_req_setup *req) {
  if(usb_mass_storage_bbb_setup(&usb_mass_storage_state, req))
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
  // Run core at 48 MHz fCLK.
  CPUCS = _CLKSPD1;

  // Use newest chip features.
  REVCTL = _ENH_PKT|_DYN_OUT;

  // NAK all transfers.
  SYNCDELAY;
  FIFORESET = _NAKALL;

  // EP2 is configured as 512-byte double buffed BULK OUT.
  EP2CFG  =  _VALID|_TYPE1|_BUF1;
  EP2CS   = 0;
  // EP6 is configured as 512-byte double buffed BULK IN.
  EP6CFG  =  _VALID|_DIR|_TYPE1|_BUF1;
  EP6CS   = 0;
  // EP4/8 are not used.
  EP4CFG &= ~_VALID;
  EP8CFG &= ~_VALID;

  // Enable IN-BULK-NAK interrupt for EP6.
  IBNIE = _IBNI_EP6;
  NAKIE = _IBN;

  // Reset and prime EP2, and reset EP6.
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

  // Re-enumerate, to make sure our descriptors are picked up correctly.
  usb_init(/*reconnect=*/true);

  while(1) {
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
  }
}
