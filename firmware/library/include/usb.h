#ifndef USB_H
#define USB_H

#include <stdint.h>

enum usb_direction {
  USB_DIR_OUT       = 0b00000000,
  USB_DIR_IN        = 0b10000000,

  USB_DIR_MASK      = 0b10000000,
};

enum usb_type {
  USB_TYPE_STANDARD = 0b00000000,
  USB_TYPE_CLASS    = 0b00100000,
  USB_TYPE_VENDOR   = 0b01000000,

  USB_TYPE_MASK     = 0b01100000,
};

enum usb_recipient {
  USB_RECIP_DEVICE  = 0b00000000,
  USB_RECIP_IFACE   = 0b00000001,
  USB_RECIP_ENDPT   = 0b00000010,
  USB_RECIP_OTHER   = 0b00000011,

  USB_RECIP_MASK    = 0b00001111,
};

enum usb_request {
  USB_REQ_GET_STATUS        =  0,
  USB_REQ_CLEAR_FEATURE     =  1,
  USB_REQ_SET_FEATURE       =  3,
  USB_REQ_SET_ADDRESS       =  5,
  USB_REQ_GET_DESCRIPTOR    =  6,
  USB_REQ_SET_DESCRIPTOR    =  7,
  USB_REQ_GET_CONFIGURATION =  8,
  USB_REQ_SET_CONFIGURATION =  9,
  USB_REQ_GET_INTERFACE     = 10,
  USB_REQ_SET_INTERFACE     = 11,
  USB_REQ_SYNCH_FRAME       = 12,
};

enum usb_descriptor {
  USB_DESC_DEVICE           = 1,
  USB_DESC_CONFIGURATION    = 2,
  USB_DESC_STRING           = 3,
  USB_DESC_INTERFACE        = 4,
  USB_DESC_ENDPOINT         = 5,
  USB_DESC_DEVICE_QUALIFIER = 6,
  USB_DESC_OTHER_SPEED_CONFIGURATION = 7,
  USB_DESC_INTERFACE_POWER  = 8,
};

enum usb_feature {
  USB_FEAT_DEVICE_REMOTE_WAKEUP = 1,
  USB_FEAT_ENDPOINT_HALT    = 0,
  USB_FEAT_TEST_MODE        = 2,
};

enum usb_attributes {
  USB_ATTR_RESERVED_1       = 0b10000000,
  USB_ATTR_SELF_POWERED     = 0b01000000,
  USB_ATTR_REMOTE_WAKEUP    = 0b00100000,
};

enum usb_transfer_type {
  USB_XFER_CONTROL          = 0b00000000,
  USB_XFER_ISOCHRONOUS      = 0b00000001,
  USB_XFER_BULK             = 0b00000010,
  USB_XFER_INTERRUPT        = 0b00000011,

  USB_XFER_MASK             = 0b00000011,
};

enum usb_synchronization_type {
  USB_SYNC_NONE             = 0b00000000,
  USB_SYNC_ASYNCHRONOUS     = 0b00000100,
  USB_SYNC_ADAPTIVE         = 0b00001000,
  USB_SYNC_SYNCHRONOUS      = 0b00001100,

  USB_SYNC_MASK             = 0b00001100,
};

enum usb_usage_type {
  USB_USAGE_DATA            = 0b00000000,
  USB_USAGE_FEEDBACK        = 0b00010000,
  USB_USAGE_IMPLICIT_FEEDBACK_DATA = 0b00100000,

  USB_USAGE_MASK            = 0b00110000,
};

struct usb_req_setup {
  uint8_t bmRequestType;
  uint8_t bRequest;
  uint16_t wValue;
  uint16_t wIndex;
  uint16_t wLength;
};

struct usb_desc_device {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint16_t bcdUSB;
  uint8_t bDeviceClass;
  uint8_t bDeviceSubClass;
  uint8_t bDeviceProtocol;
  uint8_t bMaxPacketSize0;
  uint16_t idVendor;
  uint16_t idProduct;
  uint16_t bcdDevice;
  uint8_t iManufacturer;
  uint8_t iProduct;
  uint8_t iSerialNumber;
  uint8_t bNumConfigurations;
};

struct usb_desc_device_qualifier {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint16_t bcdUSB;
  uint8_t bDeviceClass;
  uint8_t bDeviceSubClass;
  uint8_t bDeviceProtocol;
  uint8_t bMaxPacketSize0;
  uint8_t bNumConfigurations;
  uint8_t bReserved;
};

struct usb_desc_configuration {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint16_t wTotalLength;
  uint8_t bNumInterfaces;
  uint8_t bConfigurationValue;
  uint8_t iConfiguration;
  uint8_t bmAttributes;
  uint8_t bMaxPower;
};

struct usb_desc_interface {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint8_t bInterfaceNumber;
  uint8_t bAlternateSetting;
  uint8_t bNumEndpoints;
  uint8_t bInterfaceClass;
  uint8_t bInterfaceSubClass;
  uint8_t bInterfaceProtocol;
  uint8_t iInterface;
};

struct usb_desc_endpoint {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint8_t bEndpointAddress;
  uint8_t bmAttributes;
  uint16_t wMaxPacketSize;
  uint8_t bInterval;
};

struct usb_desc_langid {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint16_t wLANGID[];
};

struct usb_desc_string {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint8_t bString[];
};

#endif
