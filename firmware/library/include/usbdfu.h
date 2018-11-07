#ifndef USBDFU_H
#define USBDFU_H

#include <stdint.h>

enum /*usb_descriptor*/ {
  USB_DESC_DFU_FUNCTIONAL = 0x21,
};

enum usb_dfu_attributes {
  USB_DFU_ATTR_CAN_DNLOAD             = 0b00000001,
  USB_DFU_ATTR_CAN_UPLOAD             = 0b00000010,
  USB_DFU_ATTR_MANIFESTATION_TOLERANT = 0b00000100,
  USB_DFU_ATTR_WILL_DETACH            = 0b00001000,
};

struct usb_dfu_desc_functional {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint8_t bmAttributes;
  uint16_t wDetachTimeOut;
  uint16_t wTransferSize;
  uint16_t bcdDFUVersion;
};

typedef __code const struct usb_dfu_desc_functional
  usb_dfu_desc_functional_c;

enum {
  USB_IFACE_SUBCLASS_DFU          = 0x01,

  USB_IFACE_PROTOCOL_DFU_RUNTIME  = 0x01,
  USB_IFACE_PROTOCOL_DFU_UPGRADE  = 0x02,
};

enum usb_dfu_request {
  USB_DFU_REQ_DETACH    = 0,
  USB_DFU_REQ_DNLOAD    = 1,
  USB_DFU_REQ_UPLOAD    = 2,
  USB_DFU_REQ_GETSTATUS = 3,
  USB_DFU_REQ_CLRSTATUS = 4,
  USB_DFU_REQ_GETSTATE  = 5,
  USB_DFU_REQ_ABORT     = 6,
};

enum usb_dfu_state {
  /// Device is running its normal application.
  USB_DFU_STATE_appIDLE                 = 0,
  /// Device is running its normal application, has received the DFU_DETACH request, and
  /// is waiting for a USB reset.
  USB_DFU_STATE_appDETACH               = 1,
  /// Device is operating in the DFU mode and is waiting for requests.
  USB_DFU_STATE_dfuIDLE                 = 2,
  /// Device has received a block and is waiting for the host to solicit the status
  /// via DFU_GETSTATUS.
  USB_DFU_STATE_dfuDNLOAD_SYNC          = 3,
  /// Device is programming a control-write block into its nonvolatile memories.
  USB_DFU_STATE_dfuDNBUSY               = 4,
  /// Device is processing a download operation. Expecting DFU_DNLOAD requests.
  USB_DFU_STATE_dfuDNLOAD_IDLE          = 5,
  /// Device has received the final block of firmware from the host and is waiting for receipt of
  /// DFU_GETSTATUS to begin the Manifestation phase; or device has completed the Manifestation
  /// phase and is waiting for receipt of DFU_GETSTATUS. (Devices that can enter this state after
  /// the Manifestation phase set bmAttributes bit bitManifestationTolerant to 1.)
  USB_DFU_STATE_dfuMANIFEST_SYNC        = 6,
  /// Device is in the Manifestation phase. (Not all devices will be able to respond to
  /// DFU_GETSTATUS when in this state.)
  USB_DFU_STATE_dfuMANIFEST             = 7,
  /// Device has programmed its memories and is waiting for a USB reset or a power on reset.
  /// (Devices that must enter this state clear bitManifestationTolerant to 0.)
  USB_DFU_STATE_dfuMANIFEST_WAIT_RESET  = 8,
  /// The device is processing an upload operation. Expecting DFU_UPLOAD requests.
  USB_DFU_STATE_dfuUPLOAD_IDLE          = 9,
  /// An error has occurred. Awaiting the DFU_CLRSTATUS request.
  USB_DFU_STATE_dfuERROR                = 10,
};

enum usb_dfu_status {
  /// No error condition is present.
  USB_DFU_STATUS_OK               = 0x00,
  /// File is not targeted for use by this device.
  USB_DFU_STATUS_errTARGET        = 0x01,
  /// File is for this device but fails some vendor-specific verification test.
  USB_DFU_STATUS_errFILE          = 0x02,
  /// Device is unable to write memory.
  USB_DFU_STATUS_errWRITE         = 0x03,
  /// Memory erase function failed.
  USB_DFU_STATUS_errERASE         = 0x04,
  /// Memory erase check failed.
  USB_DFU_STATUS_errCHECK_ERASED  = 0x05,
  /// Program memory function failed.
  USB_DFU_STATUS_errPROG          = 0x06,
  /// Programmed memory failed verification.
  USB_DFU_STATUS_errVERIFY        = 0x07,
  /// Cannot program memory due to received address that is out of range.
  USB_DFU_STATUS_errADDRESS       = 0x08,
  /// Received DFU_DNLOAD with wLength = 0, but device does not think it has all of the data yet.
  USB_DFU_STATUS_errNOTDONE       = 0x09,
  /// Device’s firmware is corrupt. It cannot return to run-time (non-DFU) operations.
  USB_DFU_STATUS_errFIRMWARE      = 0x0A,
  /// iString indicates a vendor-specific error.
  USB_DFU_STATUS_errVENDOR        = 0x0B,
  /// Device detected unexpected USB reset signaling.
  USB_DFU_STATUS_errUSBR          = 0x0C,
  /// Device detected unexpected power on reset.
  USB_DFU_STATUS_errPOR           = 0x0D,
  /// Something went wrong, but the device does not know what it was.
  USB_DFU_STATUS_errUNKNOWN       = 0x0E,
  /// Device stalled an unexpected request.
  USB_DFU_STATUS_errSTALLEDPKT    = 0x0F,
};

#define USB_DFU_STATUS_NAMES \
  "No error condition is present", \
  "File is not targeted for use by this device", \
  "File is for this device but fails some vendor-specific verification test", \
  "Device is unable to write memory", \
  "Memory erase function failed", \
  "Memory erase check failed", \
  "Program memory function failed", \
  "Programmed memory failed verification", \
  "Cannot program memory due to received address that is out of range", \
  "Received DFU_DNLOAD with wLength = 0, but device does not think it has all of the data yet", \
  "Device’s firmware is corrupt. It cannot return to run-time (non-DFU) operations", \
  "iString indicates a vendor-specific error", \
  "Device detected unexpected USB reset signaling", \
  "Device detected unexpected power on reset", \
  "Something went wrong, but the device does not know what it was", \
  "Device stalled an unexpected request"

typedef enum usb_dfu_status
  usb_dfu_status_t;

struct usb_dfu_req_get_status {
  uint8_t bStatus;
  uint8_t bwPollTimeout;
  uint16_t bwPollTimeoutHigh;
  uint8_t bState;
  uint8_t iString;
};

#endif
