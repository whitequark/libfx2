#include <fx2usbmassstor.h>

#pragma save
#pragma nooverlay
bool usb_mass_storage_bbb_setup(usb_mass_storage_bbb_state_t *state,
                                __xdata struct usb_req_setup *request) {
  if(request->bmRequestType == (USB_RECIP_IFACE|USB_TYPE_CLASS|USB_DIR_OUT) &&
     request->bRequest == USB_REQ_MASS_STORAGE_BOMSR &&
     request->wValue == 0 && request->wIndex == state->interface && request->wLength == 0) {
    state->_state = USB_MASS_STORAGE_BBB_STATE_COMMAND;
    ACK_EP0();
    return true;
  }

  // USB MS BBB 3.2 says that "devices that do not support multiple LUNs may STALL this command",
  // but actually doing this appears to crash Linux's USB stack so hard that HC dies. Sigh.
  if(request->bmRequestType == (USB_RECIP_IFACE|USB_TYPE_CLASS|USB_DIR_IN) &&
     request->bRequest == USB_REQ_MASS_STORAGE_GET_MAX_LUN &&
     request->wValue == 0 && request->wIndex == state->interface && request->wLength == 1) {
    EP0BUF[0] = state->max_lun;
    SETUP_EP0_BUF(1);
    return true;
  }

  return false;
}
#pragma restore

bool usb_mass_storage_bbb_bulk_out(usb_mass_storage_bbb_state_t *state,
                                   __xdata const uint8_t *data,
                                   uint16_t length) {
  usb_mass_storage_cbw_t *cbw = (usb_mass_storage_cbw_t *)data;

  if(state->_state == USB_MASS_STORAGE_BBB_STATE_COMMAND) {
    // USB MS BBB 6.2.1: check for Valid CBW.
    if(length != sizeof(struct usb_mass_storage_cbw))
      return false;
    if(cbw->dCBWSignature != USB_MASS_STORAGE_CBW_SIGNATURE)
      return false;

    // USB MS BBB 6.2.2: check for Meaningful CBW.
    // USB MS BBB 6.4 says that "the response of a device to a CBW that is not meaningful
    /// is not specified"; we opt to treat such CBWs the same as CBWs that are not valid.
    if(cbw->bmCBWFlags & USB_MASS_STORAGE_CBW_RESERVED_FLAGS)
      return false;
    if(cbw->bCBWCBLength > 16)
      return false;
    if(cbw->bCBWLUN > state->max_lun)
      return false;

    state->_success = state->command(cbw->bCBWLUN, cbw->CBWCB, cbw->bCBWCBLength);

    state->_tag = cbw->dCBWTag;
    state->_lun = cbw->bCBWLUN;
    state->_residue   = 0;
    if(cbw->dCBWDataTransferLength == 0) {
      // USB MS BBB 5.1: "if [dCBWDataTransferLength] is zero, the device and the host
      // shall transfer no data between the CBW and the associated CSW, and the device
      // shall ignore the value of the Direction bit in bmCBWFlags.
      state->_state       = USB_MASS_STORAGE_BBB_STATE_STATUS;
    } else {
      state->_data_in     = !!(cbw->bmCBWFlags & USB_MASS_STORAGE_CBW_FLAG_DATA_IN);
      state->_data_length = cbw->dCBWDataTransferLength;
      if(state->_success) {
        state->_state     = state->_data_in ? USB_MASS_STORAGE_BBB_STATE_DATA_IN
                                            : USB_MASS_STORAGE_BBB_STATE_DATA_OUT;
      } else {
        state->_residue   = state->_data_length;
        state->_state     = state->_data_in ? USB_MASS_STORAGE_BBB_STATE_FAIL_IN
                                            : USB_MASS_STORAGE_BBB_STATE_FAIL_OUT;
      }
    }
    return true;
  }

  if(state->_state == USB_MASS_STORAGE_BBB_STATE_DATA_OUT ||
     state->_state == USB_MASS_STORAGE_BBB_STATE_FAIL_OUT) {
    if(state->_state == USB_MASS_STORAGE_BBB_STATE_DATA_OUT) {
      if(!state->data_out(state->_lun, data, length)) {
        state->_residue = state->_data_length;
        state->_success = false;
        state->_state   = USB_MASS_STORAGE_BBB_STATE_FAIL_OUT;
      }
    }
    state->_data_length -= length;
    if(state->_data_length == 0) {
      state->_state = USB_MASS_STORAGE_BBB_STATE_STATUS;
    }
    return true;
  }

  return false;
}

bool usb_mass_storage_bbb_bulk_in(usb_mass_storage_bbb_state_t *state,
                                  __xdata uint8_t *data,
                                  __xdata uint16_t *length) {
  if(state->_state == USB_MASS_STORAGE_BBB_STATE_DATA_IN ||
     state->_state == USB_MASS_STORAGE_BBB_STATE_FAIL_IN) {
    *length = (state->_data_length > state->max_in_size)
              ? state->max_in_size : state->_data_length;
    if(state->_state == USB_MASS_STORAGE_BBB_STATE_DATA_IN) {
      if(!state->data_in(state->_lun, data, *length)) {
        state->_residue = state->_data_length;
        state->_success = false;
        state->_state   = USB_MASS_STORAGE_BBB_STATE_FAIL_IN;
      }
    }
    state->_data_length -= *length;
    if(state->_data_length == 0) {
      state->_state = USB_MASS_STORAGE_BBB_STATE_STATUS;
    }
    return true;
  }

  if(state->_state == USB_MASS_STORAGE_BBB_STATE_STATUS) {
    usb_mass_storage_csw_t *csw = (usb_mass_storage_csw_t *)data;
    *length = sizeof(struct usb_mass_storage_csw);
    csw->dCSWSignature    = USB_MASS_STORAGE_CSW_SIGNATURE;
    csw->dCSWTag          = state->_tag;
    csw->dCSWDataResidue  = state->_residue;
    csw->bCSWStatus       = state->_success ? USB_MASS_STORAGE_CSW_STATUS_PASSED
                                            : USB_MASS_STORAGE_CSW_STATUS_FAILED;
    state->_state = USB_MASS_STORAGE_BBB_STATE_COMMAND;
    return true;
  }

  return false;
}
