#ifndef USBWEB_H
#define USBWEB_H

#include <usb.h>

// {3408b638-09a9-47a0-8bfd-a0768815b665}
#define USB_PLATFORM_CAPABILITY_UUID_WEBUSB \
  {0x38, 0xB6, 0x08, 0x34, 0xA9, 0x09, 0xA0, 0x47, 0x8B, 0xFD, 0xA0, 0x76, 0x88, 0x15, 0xB6, 0x65}

struct usb_desc_platform_capability_webusb {
  uint8_t  bLength;
  uint8_t  bDescriptorType;
  uint8_t  bDevCapabilityType;
  uint8_t  bReserved;
  uint8_t  PlatformCapablityUUID[16];
  uint16_t bcdVersion;
  uint8_t  bVendorCode;
  uint8_t  iLandingPage;
};

typedef __code const struct usb_desc_platform_capability_webusb
  usb_desc_platform_capability_webusb_c;

#endif
