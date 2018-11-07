#ifndef USBMASSSTOR_H
#define USBMASSSTOR_H

#include <stdint.h>

enum {
  USB_IFACE_CLASS_MASS_STORAGE              = 0x08,

  USB_IFACE_SUBCLASS_MASS_STORAGE_RBC       = 0x01,
  USB_IFACE_SUBCLASS_MASS_STORAGE_ATAPI     = 0x02,
  USB_IFACE_SUBCLASS_MASS_STORAGE_UFI       = 0x04,
  USB_IFACE_SUBCLASS_MASS_STORAGE_SCSI      = 0x06,
  USB_IFACE_SUBCLASS_MASS_STORAGE_LSD_FS    = 0x07,
  USB_IFACE_SUBCLASS_MASS_STORAGE_IEEE_1667 = 0x08,

  USB_IFACE_PROTOCOL_MASS_STORAGE_CBI_COMPL = 0x00,
  USB_IFACE_PROTOCOL_MASS_STORAGE_CBI       = 0x01,
  USB_IFACE_PROTOCOL_MASS_STORAGE_BBB       = 0x50,
  USB_IFACE_PROTOCOL_MASS_STORAGE_UAS       = 0x62,
};

enum usb_mass_storage_request {
  USB_REQ_MASS_STORAGE_ADSC                 = 0x00,
  USB_REQ_MASS_STORAGE_GET_REQUESTS         = 0xfc,
  USB_REQ_MASS_STORAGE_PUT_REQUESTS         = 0xfd,
  USB_REQ_MASS_STORAGE_GET_MAX_LUN          = 0xfe,
  USB_REQ_MASS_STORAGE_BOMSR                = 0xff,
};

enum {
  USB_MASS_STORAGE_CBW_SIGNATURE            = 0x43425355,
};

enum usb_mass_storage_cbw_flags {
  USB_MASS_STORAGE_CBW_FLAG_DATA_IN         = 0b10000000,
  USB_MASS_STORAGE_CBW_RESERVED_FLAGS       = 0b01111111,
};

struct usb_mass_storage_cbw {
  uint32_t dCBWSignature;
  uint32_t dCBWTag;
  uint32_t dCBWDataTransferLength;
  uint8_t  bmCBWFlags;
  uint8_t  bCBWLUN;
  uint8_t  bCBWCBLength;
  uint8_t  CBWCB[16];
};

typedef __xdata struct usb_mass_storage_cbw
  usb_mass_storage_cbw_t;

enum {
  USB_MASS_STORAGE_CSW_SIGNATURE            = 0x53425355,
};

enum usb_mass_storage_csw_status {
  USB_MASS_STORAGE_CSW_STATUS_PASSED        = 0x00,
  USB_MASS_STORAGE_CSW_STATUS_FAILED        = 0x01,
  USB_MASS_STORAGE_CSW_STATUS_PHASE_ERROR   = 0x02,
};

struct usb_mass_storage_csw {
  uint32_t dCSWSignature;
  uint32_t dCSWTag;
  uint32_t dCSWDataResidue;
  uint8_t  bCSWStatus;
};

typedef __xdata struct usb_mass_storage_csw
  usb_mass_storage_csw_t;

#endif
