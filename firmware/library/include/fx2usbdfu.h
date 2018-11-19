#ifndef FX2USBDFU_H
#define FX2USBDFU_H

#include <fx2usb.h>
#include <usbdfu.h>

/// State of an USB Device Firmware Upgrade interface.
struct usb_dfu_iface_state {
  /**
   * The bInterfaceNumber field corresponding to this interface in runtime mode.
   * (In upgrade mode, only one interface must be exported, so this field is ignored.)
   */
  uint8_t interface;

  /**
   * Firmware upload function. This function reads the firmware block at ``offset``
   * of requested ``length`` into ``data``. When end of firmware is reached, this function
   * should report this a block size shorter than provided ``length`` by changing it.
   *
   * The ``offset`` argument is maintained internally by this library and is increased after
   * each ``firmware_upload`` call by ``length``; it is not related to the ``wBlockNum`` field
   * from ``DFU_UPLOAD`` request.
   *
   * The firmware read out by this function must be in a format identical to that accepted
   * by the ``firmware_dnload`` function to be DFU compliant.
   *
   * This function should return ``USB_DFU_STATUS_OK`` if the firmware could be read,
   * and one of the other ``enum usb_dfu_status`` values otherwise.
   */
  usb_dfu_status_t (*firmware_upload)(uint32_t offset, __xdata uint8_t *data,
                                      __xdata uint16_t *length) __reentrant;

  /**
   * Firmware download function. This function writes the firmware block at ``offset``
   * from ``data`` that is ``length`` bytes long, which may be no greater than
   * ``wTransferSize`` field in the DFU functional descriptor. If this function receives
   * a block lenght of zero, it must validate whether it has received the complete firmware.
   *
   * The ``offset`` argument is maintained internally by this library and is increased after
   * each ``firmware_dnload`` call by ``length``; it is not related to the ``wBlockNum`` field
   * from ``DFU_DNLOAD`` request.
   *
   * This function should return ``USB_DFU_STATUS_OK`` if the firmware could be written
   * (or, in case of ``length == 0``, if the complete firmware was downloaded), and one
   * of the other ``enum usb_dfu_status`` values otherwise.
   */
  usb_dfu_status_t (*firmware_dnload)(uint32_t offset, __xdata uint8_t *data,
                                      uint16_t length) __reentrant;

  /**
   * Firmware manifestation function. This function could perform any application-specific
   * actions required to finish updating firmware, such as changing the boot address.
   *
   * This function should return ``USB_DFU_STATUS_OK`` if the firmware could be manifested,
   * and one of the other ``enum usb_dfu_status`` values otherwise.
   *
   * If this callback is set to ``NULL``, the behavior is the same as if the callback was
   * implemented as an empty function returning ``USB_DFU_STATUS_OK``.
   */
  usb_dfu_status_t (*firmware_manifest)() __reentrant;

  /// State of the DFU interface, as per DFU specification.
  volatile enum usb_dfu_state state;

#ifndef DOXYGEN
  // Private fields, subject to change at any time.
  volatile enum usb_dfu_status status;
#if __SDCC_VERSION_MAJOR > 3 || __SDCC_VERSION_MINOR >= 7
  volatile bool pending, sync;
#else
  volatile uint8_t pending, sync;
#endif
  uint16_t length;
  uint32_t offset;
#endif
};

typedef __xdata struct usb_dfu_iface_state
  usb_dfu_iface_state_t;

/**
 * Handle USB Device Firmware Update interface SETUP packets.
 * This function makes the appropriate changes to the state and returns ``true`` if a SETUP packet
 * addressed this interface, or returns ``false`` otherwise.
 */
bool usb_dfu_setup(usb_dfu_iface_state_t *state, __xdata struct usb_req_setup *request);

/**
 * Handle USB Device Firmware Update interface SETUP packets that perform lengthy operations
 * (i.e. actual firmware upload/download).
 */
void usb_dfu_setup_deferred(usb_dfu_iface_state_t *state);

/**
 * The global DFU configuration, defined in the application code.
 * It only makes sense for a single device to expose a single DFU interface at a time.
 */
extern usb_dfu_iface_state_t usb_dfu_iface_state;

#endif
