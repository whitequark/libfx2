#ifndef USBCDC_H
#define USBCDC_H

enum {
  /// Communications Device Class
  USB_DEV_CLASS_CDC   = 0x02,

  /// Communications Interface Class
  USB_IFACE_CLASS_CIC = 0x02,
  /// Data Interface Class
  USB_IFACE_CLASS_DIC = 0x0A,

  /// Direct Line Control Model Interface Subclass
  USB_IFACE_SUBCLASS_CDC_CIC_DLCM = 0x01,
  /// Abstract Control Model Interface Subclass
  USB_IFACE_SUBCLASS_CDC_CIC_ACM = 0x02,
  /// Telephone Control Model Interface Subclass
  USB_IFACE_SUBCLASS_CDC_CIC_TCM = 0x03,
  /// Multi-Channel Control Model Interface Subclass
  USB_IFACE_SUBCLASS_CDC_CIC_MCCM = 0x04,
  /// CAPI Control Model Interface Subclass
  USB_IFACE_SUBCLASS_CDC_CIC_CAPICM = 0x05,
  /// Ethernet Networking Control Model Interface Subclass
  USB_IFACE_SUBCLASS_CDC_CIC_ENCM = 0x06,
  /// ATM Networking Control Model Interface Subclass
  USB_IFACE_SUBCLASS_CDC_CIC_ATMNCM = 0x07,
  /// Wireless Handset Control Model Interface Subclass
  USB_IFACE_SUBCLASS_CDC_CIC_WHCM = 0x08,
  /// Device Management Interface Subclass
  USB_IFACE_SUBCLASS_CDC_CIC_DEV_MGMT = 0x09,
  /// Mobile Direct Line Model Interface Subclass
  USB_IFACE_SUBCLASS_CDC_CIC_MDLM = 0x0A,
  /// OBEX Interface Subclass
  USB_IFACE_SUBCLASS_CDC_CIC_OBEX = 0x0B,
  /// Ethernet Emulation Model Interface Subclass
  USB_IFACE_SUBCLASS_CDC_CIC_EEM = 0x0C,
  /// Network Control Model Interface Subclass
  USB_IFACE_SUBCLASS_CDC_CIC_NCM = 0x0D,

  /// Communications Interface Protocol: No class specific protocol required
  USB_IFACE_PROTOCOL_CDC_CIC_NONE = 0x00,
  /// Communications Interface Protocol: AT Commands: V.250 etc
  USB_IFACE_PROTOCOL_CDC_CIC_AT_V250 = 0x01,
  /// Communications Interface Protocol: AT Commands defined by PCCA-101
  USB_IFACE_PROTOCOL_CDC_CIC_AT_PCCA_101 = 0x02,
  /// Communications Interface Protocol: AT Commands defined by PCCA-101 & Annex O
  USB_IFACE_PROTOCOL_CDC_CIC_AT_PCCA_101_ANNEX_O = 0x03,
  /// Communications Interface Protocol: AT Commands defined by GSM 07.07
  USB_IFACE_PROTOCOL_CDC_CIC_GSM_07_07 = 0x04,
  /// Communications Interface Protocol: AT Commands defined by 3GPP 27.007
  USB_IFACE_PROTOCOL_CDC_CIC_3GPP_27_007 = 0x05,
  /// Communications Interface Protocol: AT Commands defined by TIA for CDMA
  USB_IFACE_PROTOCOL_CDC_CIC_TIA_CDMA = 0x06,
  /// Communications Interface Protocol: Ethernet Emulation Model
  USB_IFACE_PROTOCOL_CDC_CIC_EEM = 0x07,
  /// Communications Interface Protocol: Commands defined by Command Set Functional Descriptor
  USB_IFACE_PROTOCOL_CDC_CIC_EXTERNAL = 0xFE,

  /// Data Interface Subclass
  USB_IFACE_SUBCLASS_CDC_DIC = 0x00,

  /// Data Interface Protocol: No class specific protocol required
  USB_IFACE_PROTOCOL_CDC_DIC_NONE = 0x00,
  /// Data Interface Protocol: Network Transfer Block
  USB_IFACE_PROTOCOL_CDC_DIC_NTB = 0x01,
  /// Data Interface Protocol: Physical interface protocol for ISDN BRI
  USB_IFACE_PROTOCOL_CDC_DIC_I_430 = 0x30,
  /// Data Interface Protocol: HDLC
  USB_IFACE_PROTOCOL_CDC_DIC_HDLC = 0x31,
  /// Data Interface Protocol: Transparent
  USB_IFACE_PROTOCOL_CDC_DIC_TRANSPARENT = 0x32,
  /// Data Interface Protocol: Management protocol for Q.921 data link protocol
  USB_IFACE_PROTOCOL_CDC_DIC_Q_921M = 0x50,
  /// Data Interface Protocol: Data link protocol for Q.931
  USB_IFACE_PROTOCOL_CDC_DIC_Q_921D = 0x51,
  /// Data Interface Protocol: TEI-multiplexor for Q.921 data link protocol
  USB_IFACE_PROTOCOL_CDC_DIC_Q_921TM = 0x52,
  /// Data Interface Protocol: Data compression procedures
  USB_IFACE_PROTOCOL_CDC_DIC_V_42BIS = 0x90,
  /// Data Interface Protocol: Euro-ISDN protocol control
  USB_IFACE_PROTOCOL_CDC_DIC_Q_931_EURO_ISDN = 0x91,
  /// Data Interface Protocol: V.24 rate adaptation to ISDN
  USB_IFACE_PROTOCOL_CDC_DIC_V_120 = 0x92,
  /// Data Interface Protocol: CAPI Commands
  USB_IFACE_PROTOCOL_CDC_DIC_CAPI_2_0 = 0x93,
  /// Data Interface Protocol: Host based driver.
  USB_IFACE_PROTOCOL_CDC_DIC_HOST_BASED = 0xFD,
  /// Data Interface Protocol: Protocol(s) defined by Protocol Unit Functional Descriptors
  /// on Communications Class Interface
  USB_IFACE_PROTOCOL_CDC_DIC_EXTERNAL = 0xFE,
};

enum /*usb_descriptor*/ {
  USB_DESC_CS_INTERFACE = 0x24,
  USB_DESC_CS_ENDPOINT  = 0x25,
};

enum usb_cdc_desc_functional_subtype {
  /// Header Functional Descriptor
  USB_DESC_CDC_FUNCTIONAL_SUBTYPE_HEADER = 0x00,
  /// Call Management Functional Descriptor
  USB_DESC_CDC_FUNCTIONAL_SUBTYPE_CALL_MGMT = 0x01,
  /// Abstract Control Management Functional Descriptor
  USB_DESC_CDC_FUNCTIONAL_SUBTYPE_ACM = 0x02,
  /// Union Functional Descriptor
  USB_DESC_CDC_FUNCTIONAL_SUBTYPE_UNION = 0x06,
};

struct usb_cdc_desc_functional_header {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint8_t bDescriptorSubType;
  uint16_t bcdCDC;
};

typedef __code const struct usb_cdc_desc_functional_header
  usb_cdc_desc_functional_header_c;

enum {
  /// Not set - Device does not handle call management itself.
  /// Set - Device handles call management itself.
  USB_CDC_CALL_MGMT_CAP_HANDLES_CALL_MGMT   = 0b00000001,
  /// Not set - Device sends/receives call management information only over the Communications
  /// Class interface.
  /// Set - Device can send/receive call management information over a Data Class interface.
  USB_CDC_CALL_MGMT_CAP_CALL_MGMT_OVER_DATA = 0b00000010,
};

struct usb_cdc_desc_functional_call_mgmt {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint8_t bDescriptorSubType;
  uint8_t bmCapabilities;
  uint8_t bDataInterface;
};

typedef __code const struct usb_cdc_desc_functional_call_mgmt
  usb_cdc_desc_functional_call_mgmt_c;

enum {
  /// Set - Device supports the request combination of Set_Comm_Feature, Clear_Comm_Feature, and
  /// Get_Comm_Feature.
  USB_CDC_ACM_CAP_REQ_COMM_FEATURE        = 0b00000001,
  /// Set - Device supports the request combination of Set_Line_Coding, Set_Control_Line_State,
  /// Get_Line_Coding, and the notification Serial_State.
  USB_CDC_ACM_CAP_REQ_LINE_CODING_STATE   = 0b00000010,
  /// Set - Device supports the request Send_Break
  USB_CDC_ACM_CAP_REQ_SEND_BREAK          = 0b00000100,
  /// Set - Device supports the notification Network_Connection.
  USB_CDC_ACM_CAP_REQ_NETWORK_CONNECTION  = 0b00001000,
};

struct usb_cdc_desc_functional_acm {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint8_t bDescriptorSubType;
  uint8_t bmCapabilities;
};

typedef __code const struct usb_cdc_desc_functional_acm
  usb_cdc_desc_functional_acm_c;

struct usb_cdc_desc_functional_union {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint8_t bDescriptorSubType;
  uint8_t bControlInterface;
  uint8_t bSubordinateInterface[];
};

typedef __code const struct usb_cdc_desc_functional_union
  usb_cdc_desc_functional_union_c;

/// Class-Specific Request Codes
enum usb_cdc_request {
  USB_CDC_REQ_SEND_ENCAPSULATED_COMMAND   = 0x00,
  USB_CDC_REQ_GET_ENCAPSULATED_RESPONSE   = 0x01,
};

/// Class-Specific Request Codes for PSTN subclasses
enum usb_cdc_pstn_request {
  USB_CDC_PSTN_REQ_SET_COMM_FEATURE       = 0x02,
  USB_CDC_PSTN_REQ_GET_COMM_FEATURE       = 0x03,
  USB_CDC_PSTN_REQ_CLEAR_COMM_FEATURE     = 0x04,
  USB_CDC_PSTN_REQ_SET_AUX_LINE_STATE     = 0x10,
  USB_CDC_PSTN_REQ_SET_HOOK_STATE         = 0x11,
  USB_CDC_PSTN_REQ_PULSE_SETUP            = 0x12,
  USB_CDC_PSTN_REQ_SEND_PULSE             = 0x13,
  USB_CDC_PSTN_REQ_SET_PULSE_TIME         = 0x14,
  USB_CDC_PSTN_REQ_RING_AUX_JACK          = 0x15,
  USB_CDC_PSTN_REQ_SET_LINE_CODING        = 0x20,
  USB_CDC_PSTN_REQ_GET_LINE_CODING        = 0x21,
  USB_CDC_PSTN_REQ_SET_CONTROL_LINE_STATE = 0x22,
  USB_CDC_PSTN_REQ_SEND_BREAK             = 0x23,
  USB_CDC_PSTN_REQ_SET_RINGER_PARMS       = 0x30,
  USB_CDC_PSTN_REQ_GET_RINGER_PARMS       = 0x31,
  USB_CDC_PSTN_REQ_SET_OPERATION_PARMS    = 0x32,
  USB_CDC_PSTN_REQ_GET_OPERATION_PARMS    = 0x33,
  USB_CDC_PSTN_REQ_SET_LINE_PARMS         = 0x34,
  USB_CDC_PSTN_REQ_GET_LINE_PARMS         = 0x35,
  USB_CDC_PSTN_REQ_DIAL_DIGITS            = 0x36,
};

enum {
  USB_CDC_REQ_LINE_CODING_STOP_BITS_1   = 0,
  USB_CDC_REQ_LINE_CODING_STOP_BITS_1_5 = 1,
  USB_CDC_REQ_LINE_CODING_STOP_BITS_2   = 2,

  USB_CDC_REQ_LINE_CODING_PARITY_NONE   = 0,
  USB_CDC_REQ_LINE_CODING_PARITY_ODD    = 1,
  USB_CDC_REQ_LINE_CODING_PARITY_EVEN   = 2,
  USB_CDC_REQ_LINE_CODING_PARITY_MARK   = 3,
  USB_CDC_REQ_LINE_CODING_PARITY_SPACE  = 4,
};

struct usb_cdc_req_line_coding {
  uint32_t dwDTERate;
  uint8_t bCharFormat;
  uint8_t bParityType;
  uint8_t bDataBits;
};

#endif
