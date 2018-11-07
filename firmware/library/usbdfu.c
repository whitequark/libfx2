#include <fx2lib.h>
#include <fx2usbdfu.h>

bool usb_dfu_setup(usb_dfu_iface_state_t *dfu, __xdata struct usb_req_setup *req) {
  uint8_t interface = dfu->state > USB_DFU_STATE_appDETACH ? 0 : dfu->interface;

  if((req->bmRequestType & (USB_TYPE_MASK|USB_RECIP_MASK)) != (USB_TYPE_CLASS|USB_RECIP_IFACE) ||
     req->wIndex != interface)
    return false;

  if((req->bmRequestType & USB_DIR_MASK) == USB_DIR_OUT &&
      req->bRequest == USB_DFU_REQ_DETACH &&
      req->wLength == 0 &&
      dfu->state == USB_DFU_STATE_appIDLE) {
    dfu->state = USB_DFU_STATE_appDETACH;
    ACK_EP0();
    return true;
  }

  if((req->bmRequestType & USB_DIR_MASK) == USB_DIR_IN &&
      req->bRequest == USB_DFU_REQ_GETSTATE && req->wValue == 0 &&
      req->wLength == sizeof(uint8_t)) {
    EP0BUF[0] = dfu->state;
    SETUP_EP0_BUF(1);
    return true;
  }

  if((req->bmRequestType & USB_DIR_MASK) == USB_DIR_IN &&
      req->bRequest == USB_DFU_REQ_GETSTATUS && req->wValue == 0 &&
      req->wLength == sizeof(struct usb_dfu_req_get_status)) {
    __xdata struct usb_dfu_req_get_status *status =
      (__xdata struct usb_dfu_req_get_status *)EP0BUF;

    if((dfu->state == USB_DFU_STATE_dfuDNLOAD_SYNC ||
        dfu->state == USB_DFU_STATE_dfuMANIFEST_SYNC) &&
       !dfu->sync) {
      // If we're here, then EP0BUF is still in use, but the host already sent GETSTATUS.
      // If we do SETUP_EP0_* right now, we'll overwrite EP0BUF, and get corrupted data.
      // So, delay responding to this packet until after EP0BUF is copied to scratch space.
      //
      // (GETSTATUS in dfuMANIFEST-SYNC does not have this restriction, but these requests
      // use identical flows in the DFU spec, and it is simpler to handle them the same way.)
      dfu->sync = true;
      return true;
    } else if(dfu->state == USB_DFU_STATE_dfuDNLOAD_SYNC) {
      dfu->state = USB_DFU_STATE_dfuDNBUSY;
    } else if(dfu->state == USB_DFU_STATE_dfuMANIFEST_SYNC) {
      dfu->state = USB_DFU_STATE_dfuMANIFEST;
    }

    status->bStatus = dfu->status;
    // We just need a few milliseconds to copy out EP0BUF contents to the scratch space,
    // and then we can handle requests normally. So, hardcode 10 ms here.
    status->bwPollTimeout = 10;
    status->bwPollTimeoutHigh = 0;
    status->bState = dfu->state;
    status->iString = 0;
    SETUP_EP0_BUF(sizeof(struct usb_dfu_req_get_status));
    return true;
  }

  if(dfu->state > USB_DFU_STATE_appDETACH) {
    if((req->bmRequestType & USB_DIR_MASK) == USB_DIR_OUT &&
        req->bRequest == USB_DFU_REQ_CLRSTATUS && req->wValue == 0 &&
        req->wLength == 0 &&
        dfu->state == USB_DFU_STATE_dfuERROR) {
      dfu->status = USB_DFU_STATUS_OK;
      dfu->state  = USB_DFU_STATE_dfuIDLE;
      ACK_EP0();
      return true;
    }

    if((req->bmRequestType & USB_DIR_MASK) == USB_DIR_IN &&
        req->bRequest == USB_DFU_REQ_UPLOAD) {
      if(dfu->state == USB_DFU_STATE_dfuIDLE) {
        dfu->state    = USB_DFU_STATE_dfuUPLOAD_IDLE;
        dfu->offset   = 0;
        dfu->length   = req->wLength;
        dfu->pending  = true;
        return true;
      } else if(dfu->state == USB_DFU_STATE_dfuUPLOAD_IDLE) {
        dfu->length   = req->wLength;
        dfu->pending  = true;
        return true;
      }
    }

    if((req->bmRequestType & USB_DIR_MASK) == USB_DIR_OUT &&
        req->bRequest == USB_DFU_REQ_DNLOAD) {
      if(dfu->state == USB_DFU_STATE_dfuIDLE) {
        dfu->state    = USB_DFU_STATE_dfuDNLOAD_SYNC;
        dfu->offset   = 0;
        dfu->length   = req->wLength;
        dfu->pending  = true;
        dfu->sync     = false;
        SETUP_EP0_BUF(0);
        return true;
      } else if(dfu->state == USB_DFU_STATE_dfuDNLOAD_IDLE && req->wLength > 0) {
        dfu->state    = USB_DFU_STATE_dfuDNLOAD_SYNC;
        dfu->length   = req->wLength;
        dfu->pending  = true;
        dfu->sync     = false;
        SETUP_EP0_BUF(0);
        return true;
      } else if(dfu->state == USB_DFU_STATE_dfuDNLOAD_IDLE) {
        dfu->state    = USB_DFU_STATE_dfuMANIFEST_SYNC;
        dfu->pending  = true;
        dfu->sync     = false;
        ACK_EP0();
        return true;
      }
    }

    if((req->bmRequestType & USB_DIR_MASK) == USB_DIR_OUT &&
        req->bRequest == USB_DFU_REQ_ABORT) {
      if(dfu->state == USB_DFU_STATE_dfuIDLE ||
         dfu->state == USB_DFU_STATE_dfuDNLOAD_SYNC ||
         dfu->state == USB_DFU_STATE_dfuDNLOAD_IDLE ||
         dfu->state == USB_DFU_STATE_dfuMANIFEST_SYNC ||
         dfu->state == USB_DFU_STATE_dfuUPLOAD_IDLE) {
        dfu->state = USB_DFU_STATE_dfuIDLE;
        return true;
      }
    }
  }

  dfu->status = USB_DFU_STATUS_errSTALLEDPKT;
  dfu->state  = USB_DFU_STATE_dfuERROR;
  STALL_EP0();
  return true;
}

void usb_dfu_setup_deferred(usb_dfu_iface_state_t *dfu) {
  if(dfu->pending) {
    if(dfu->state == USB_DFU_STATE_dfuUPLOAD_IDLE) {
      uint16_t length = dfu->length;
      dfu->status = dfu->firmware_upload(dfu->offset, &EP0BUF[0], &dfu->length);
      if(dfu->status == USB_DFU_STATUS_OK) {
        SETUP_EP0_BUF(dfu->length);
        if(dfu->length < length) {
          dfu->state = USB_DFU_STATE_dfuIDLE;
        }
        dfu->offset += dfu->length;
        dfu->pending = false;
        return;
      }
    } else if(dfu->state == USB_DFU_STATE_dfuDNLOAD_SYNC) {
      while(EP0CS & _BUSY);
      xmemcpy(scratch, &EP0BUF[0], dfu->length);

      // Wait until we get a GETSTATUS request (in case we still haven't got one), and then reply
      // to it from here, after we've safely stashed away EP0BUF contents.
      while(!dfu->sync);
      usb_dfu_setup(dfu, (__xdata struct usb_req_setup *)SETUPDAT);
      return;
    } else if(dfu->state == USB_DFU_STATE_dfuDNBUSY) {
      dfu->status = dfu->firmware_dnload(dfu->offset, scratch, dfu->length);
      if(dfu->status == USB_DFU_STATUS_OK) {
        dfu->offset += dfu->length;
        dfu->state = USB_DFU_STATE_dfuDNLOAD_IDLE;
        dfu->pending = false;
      }
      return;
    } else if(dfu->state == USB_DFU_STATE_dfuMANIFEST_SYNC) {
      while(!dfu->sync);
      usb_dfu_setup(dfu, (__xdata struct usb_req_setup *)SETUPDAT);
      return;
    } else if(dfu->state == USB_DFU_STATE_dfuMANIFEST) {
      if(dfu->firmware_manifest) {
        dfu->status = dfu->firmware_manifest();
      } else {
        dfu->status = USB_DFU_STATUS_OK;
      }
      if(dfu->status == USB_DFU_STATUS_OK) {
        dfu->state = USB_DFU_STATE_dfuIDLE;
        dfu->pending = false;
        return;
      }
    }

    dfu->state   = USB_DFU_STATE_dfuERROR;
    dfu->pending = false;
    STALL_EP0();
  }
}
