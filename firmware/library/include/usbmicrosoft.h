#ifndef USBMICROSOFT_H
#define USBMICROSOFT_H

#include <usb.h>

#define USB_DESC_MICROSOFT_V10_SIGNATURE \
  { 0x4D, 0x00, 0x53, 0x00, 0x46, 0x00, 0x54, 0x00, 0x31, 0x00, 0x30, 0x00, 0x30, 0x00 }

struct usb_desc_microsoft_v10 {
  uint8_t  bLength;
  uint8_t  bDescriptorType;
  uint8_t  qwSignature[14];
  uint8_t  bMS_VendorCode;
  uint8_t  bPad;
};

typedef __code const struct usb_desc_microsoft_v10
  usb_desc_microsoft_v10_c;

enum usb_descriptor_microsoft {
  USB_DESC_MS_EXTENDED_COMPAT_ID  = 0x04,
  USB_DESC_MS_EXTENDED_PROPERTIES = 0x05,
};

struct usb_desc_ms_compat_function {
  uint8_t  bFirstInterfaceNumber;
  uint8_t  bReserved1;
  uint8_t  compatibleID[8];
  uint8_t  subCompatibleID[8];
  uint8_t  bReserved[6];
};

struct usb_desc_ms_ext_compat_id {
  uint32_t dwLength;
  uint16_t bcdVersion;
  uint16_t wIndex;
  uint8_t  bCount;
  uint8_t  bReserved[7];
  struct usb_desc_ms_compat_function functions[];
};

typedef __code const struct usb_desc_ms_ext_compat_id
  usb_desc_ms_ext_compat_id_c;

#endif
