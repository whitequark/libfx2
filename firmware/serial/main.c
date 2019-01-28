// This example attempts to present a minimal USB-Serial device that is nevertheless practical,
// i.e. is compatible out of the box in all major operating systems. As described in comments
// below, this is no easy task.

// Note that the USB CDC ACM specification is simultaneously too permissive and too restricting.
// Ideally, we would use the following call management model (from USB CDC 1.2 section 3.4.1):
//
//    The device provides an internal implementation of call management over the Data Class
//    interface but not the Communications Class interface. In this case, the Communications Class
//    interface is also minimally represented and only provides device management over
//    a management element (endpoint 0). This configuration most closely corresponds to
//    the Abstract Control Model in which commands and data are multiplexed over the Data Class
//    interface. Activation of the command mode from data mode is accomplished using
//    the Heatherington Escape Sequence or the TIES method. For more information about the
//    Abstract Control Model, see the PSTN Subclass Specification.
//
// Unfortunately, other parts of the USB CDC specification as well as USB CDC PSTN specification
// actually insist on a management element (EP0) and a notification element (interrupt EP IN),
// in direct contradiction with the suggested model above. Even worse, real-world operating
// systems (such as Linux) outright reject any USB CDC ACM device that does not have both
// a management and a notification element.
//
// Therefore, in spite of declaring and implementing no out-of-band call management (as we have
// no need for call management at all), we are forced to provide an interrupt endpoint that will
// never transfer any data. It is natural to use EP1IN for this. Note that, since EP1IN and EP1OUT
// buffers are only 64 bytes long, it is not valid to use EP1IN and EP1OUT for the Data Class
// interface anyway; we only lose them for any other uses.

#include <fx2lib.h>
#include <fx2delay.h>
#include <fx2usb.h>
#include <usbcdc.h>
#include <ctype.h>

usb_desc_device_c usb_device = {
  .bLength              = sizeof(struct usb_desc_device),
  .bDescriptorType      = USB_DESC_DEVICE,
  .bcdUSB               = 0x0200,
  // It would make more sense for this to be USB_DEV_CLASS_PER_INTERFACE, such that the device
  // could be a composite device and include non-CDC interfaces. However, this does not work under
  // Windows; it enumerates a broken unknown device and a broken serial port instead. It is likely
  // that the following Microsoft documet describes a way to make it work, but I have not verified
  // it: https://docs.microsoft.com/en-us/windows-hardware/drivers/usbcon/usb-common-class-generic-parent-driver
  .bDeviceClass         = USB_DEV_CLASS_CDC,
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

usb_desc_interface_c usb_iface_cic = {
  .bLength              = sizeof(struct usb_desc_interface),
  .bDescriptorType      = USB_DESC_INTERFACE,
  .bInterfaceNumber     = 0,
  .bAlternateSetting    = 0,
  .bNumEndpoints        = 1,
  .bInterfaceClass      = USB_IFACE_CLASS_CIC,
  .bInterfaceSubClass   = USB_IFACE_SUBCLASS_CDC_CIC_ACM,
  .bInterfaceProtocol   = USB_IFACE_PROTOCOL_CDC_CIC_NONE,
  .iInterface           = 0,
};

usb_desc_endpoint_c usb_endpoint_ep1_in = {
  .bLength              = sizeof(struct usb_desc_endpoint),
  .bDescriptorType      = USB_DESC_ENDPOINT,
  .bEndpointAddress     = 1|USB_DIR_IN,
  .bmAttributes         = USB_XFER_INTERRUPT,
  .wMaxPacketSize       = 8,
  .bInterval            = 10,
};

usb_cdc_desc_functional_header_c usb_func_cic_header = {
  .bLength              = sizeof(struct usb_cdc_desc_functional_header),
  .bDescriptorType      = USB_DESC_CS_INTERFACE,
  .bDescriptorSubType   = USB_DESC_CDC_FUNCTIONAL_SUBTYPE_HEADER,
  .bcdCDC               = 0x0120,
};

usb_cdc_desc_functional_acm_c usb_func_cic_acm = {
  .bLength              = sizeof(struct usb_cdc_desc_functional_acm),
  .bDescriptorType      = USB_DESC_CS_INTERFACE,
  .bDescriptorSubType   = USB_DESC_CDC_FUNCTIONAL_SUBTYPE_ACM,
  .bmCapabilities       = 0,
};

usb_cdc_desc_functional_union_c usb_func_cic_union = {
  .bLength              = sizeof(struct usb_cdc_desc_functional_union) +
                          sizeof(uint8_t) * 1,
  .bDescriptorType      = USB_DESC_CS_INTERFACE,
  .bDescriptorSubType   = USB_DESC_CDC_FUNCTIONAL_SUBTYPE_UNION,
  .bControlInterface    = 0,
  .bSubordinateInterface = { 1 },
};

usb_desc_interface_c usb_iface_dic = {
  .bLength              = sizeof(struct usb_desc_interface),
  .bDescriptorType      = USB_DESC_INTERFACE,
  .bInterfaceNumber     = 1,
  .bAlternateSetting    = 0,
  .bNumEndpoints        = 2,
  .bInterfaceClass      = USB_IFACE_CLASS_DIC,
  .bInterfaceSubClass   = USB_IFACE_SUBCLASS_CDC_DIC,
  .bInterfaceProtocol   = USB_IFACE_PROTOCOL_CDC_DIC_NONE,
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
    .bNumInterfaces       = 2,
    .bConfigurationValue  = 1,
    .iConfiguration       = 0,
    .bmAttributes         = USB_ATTR_RESERVED_1,
    .bMaxPower            = 50,
  },
  {
    { .interface = &usb_iface_cic },
    { .generic   = (struct usb_desc_generic *) &usb_func_cic_header },
    { .generic   = (struct usb_desc_generic *) &usb_func_cic_acm },
    { .generic   = (struct usb_desc_generic *) &usb_func_cic_union },
    { .endpoint  = &usb_endpoint_ep1_in },
    { .interface = &usb_iface_dic },
    { .endpoint  = &usb_endpoint_ep2_out },
    { .endpoint  = &usb_endpoint_ep6_in },
    { 0 }
  }
};

usb_configuration_set_c usb_configs[] = {
  &usb_config,
};

usb_ascii_string_c usb_strings[] = {
  [0] = "whitequark@whitequark.org",
  [1] = "FX2 series serial interface example",
};

usb_descriptor_set_c usb_descriptor_set = {
  .device           = &usb_device,
  .config_count     = ARRAYSIZE(usb_configs),
  .configs          = usb_configs,
  .string_count     = ARRAYSIZE(usb_strings),
  .strings          = usb_strings,
};

void handle_usb_setup(__xdata struct usb_req_setup *req) {
  // We *very specifically* declare that we do not support, among others, SET_CONTROL_LINE_STATE
  // request, but Linux sends it anyway and this results in timeouts propagating to userspace.
  // Linux will send us other requests we explicitly declare to not support, but those just fail.
  if(req->bmRequestType == (USB_RECIP_IFACE|USB_TYPE_CLASS|USB_DIR_OUT) &&
     req->bRequest == USB_CDC_PSTN_REQ_SET_CONTROL_LINE_STATE &&
     req->wIndex == 0 && req->wLength == 0) {
    ACK_EP0();
    return;
  }

  // We *very specifically* declare that we do not support, among others, GET_LINE_CODING request,
  // but Windows sends it anyway and this results in errors propagating to userspace.
  if(req->bmRequestType == (USB_RECIP_IFACE|USB_TYPE_CLASS|USB_DIR_IN) &&
     req->bRequest == USB_CDC_PSTN_REQ_GET_LINE_CODING &&
     req->wIndex == 0 && req->wLength == 7) {
    __xdata struct usb_cdc_req_line_coding *line_coding =
      (__xdata struct usb_cdc_req_line_coding *)EP0BUF;
    line_coding->dwDTERate = 115200;
    line_coding->bCharFormat = USB_CDC_REQ_LINE_CODING_STOP_BITS_1;
    line_coding->bParityType = USB_CDC_REQ_LINE_CODING_PARITY_NONE;
    line_coding->bDataBits = 8;
    SETUP_EP0_BUF(sizeof(struct usb_cdc_req_line_coding));
    return;
  }

  // We *very specifically* declare that we do not support, among others, SET_LINE_CODING request,
  // but Windows sends it anyway and this results in errors propagating to userspace.
  if(req->bmRequestType == (USB_RECIP_IFACE|USB_TYPE_CLASS|USB_DIR_OUT) &&
     req->bRequest == USB_CDC_PSTN_REQ_SET_LINE_CODING &&
     req->wIndex == 0 && req->wLength == 7) {
    SETUP_EP0_BUF(0);
    return;
  }

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

  // EP1IN is configured as INTERRUPT IN.
  EP1INCFG = _VALID|_TYPE1|_TYPE0;
  // EP1OUT is not used.
  EP1OUTCFG &= ~_VALID;

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
  usb_init(/*disconnect=*/true);

  {
    uint16_t length = 0;
    while(1) {
      if(length == 0 && !(EP2CS & _EMPTY)) {
        length = (EP2BCH << 8) | EP2BCL;
        xmemcpy(scratch, EP2FIFOBUF, length);
        EP2BCL = 0;

        // Permute the buffer in an amusing way.
        {
          uint16_t i;
          for(i = 0; i < length; i++)
          {
            char c = scratch[i];
                 if(isupper(c)) c = tolower(c);
            else if(islower(c)) c = toupper(c);
            scratch[i] = c;
          }
        }
      }

      if(length != 0 && pending_ep6_in) {
        xmemcpy(EP6FIFOBUF, scratch, length);
        EP6BCH = length >> 8;
        SYNCDELAY;
        EP6BCL = length;

        length = 0;
        pending_ep6_in = false;
      }
    }
  }
}
