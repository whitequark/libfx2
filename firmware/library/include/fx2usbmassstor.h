#ifndef FX2USBMASSSTOR_H
#define FX2USBMASSSTOR_H

#include <fx2usb.h>
#include <usbmassstor.h>

/// State of an USB Mass Storage Bulk-Only Transport interface.
struct usb_mass_storage_bbb_state {
  /// The bInterfaceNumber field corresponding to this interface.
  uint8_t interface;

  /// The value of the wMaxPacketSize field of the BULK IN endpoint of this interface.
  uint16_t max_in_size;

  /// The maximum LUN number of this interface. Most interfaces will only use a single LUN,
  /// and so will have ``max_lun`` set to zero.
  uint8_t max_lun;

  /// The Command callback. This function is called for each CBW. It should return ``true``
  /// if the command is recognized and well-formed, which either proceeds to the Data callbacks
  /// (if there is any data to be transferred) or returns a Passed CSW immediately; or ``false``,
  /// in which case the data callbacks are never called and a Failed CSW is returned.
  bool (*command)(uint8_t lun, __xdata uint8_t *command, uint8_t length) __reentrant;

  /// The Data-Out callback. This function is called with chunks of data that follow a CBW
  /// that specifies an OUT transfer. It can read exactly ``length`` bytes from
  /// the ``data`` buffer. If data is processed successfully, this function should return ``true``,
  /// which will eventually return a Passed CSW to the host; otherwise, the rest of the transfer
  /// is skipped (this callback is not invoked again), and a Failed CSW is returned.
  bool (*data_out)(uint8_t lun, __xdata const uint8_t *data, uint16_t length) __reentrant;

  /// The Data-In callback. This function is called each time a chunk of data is necessary
  /// following a CBW that specifies an IN transfer. It should fill exactly ``length``
  /// bytes into the ``data`` buffer. If data is processed successfully, this function should
  /// return ``true``, which will eventually return a Passed CSW to the host; otherwise,
  /// the rest of the transfer is skipped (this callback is not invoked again), and a Failed CSW
  /// is returned.
  bool (*data_in)(uint8_t lun, __xdata uint8_t *data, uint16_t length) __reentrant;

#ifndef DOXYGEN
  // Private fields, subject to change at any time.
  enum {
    USB_MASS_STORAGE_BBB_STATE_COMMAND,
    USB_MASS_STORAGE_BBB_STATE_DATA_OUT,
    USB_MASS_STORAGE_BBB_STATE_FAIL_OUT,
    USB_MASS_STORAGE_BBB_STATE_DATA_IN,
    USB_MASS_STORAGE_BBB_STATE_FAIL_IN,
    USB_MASS_STORAGE_BBB_STATE_STATUS,
  } _state;
  uint32_t _tag;
  uint8_t  _lun;
#if __SDCC_VERSION_MAJOR > 3 || __SDCC_VERSION_MINOR >= 7
  bool     _data_in;
  bool     _success;
#else
  uint8_t  _data_in;
  uint8_t  _success;
#endif
  uint32_t _data_length;
  uint32_t _residue;
#endif
};

typedef __xdata struct usb_mass_storage_bbb_state
  usb_mass_storage_bbb_state_t;

/**
 * Handle USB Mass Storage Bulk-Only Transport interface SETUP packets.
 * This function makes the appropriate changes to the state and returns ``true`` if a SETUP packet
 * addressed this interface, or returns ``false`` otherwise.
 */
bool usb_mass_storage_bbb_setup(usb_mass_storage_bbb_state_t *state,
                                __xdata struct usb_req_setup *request);

/**
 * Process USB Mass Storage Bulk-Only Transport interface BULK OUT packets.
 *
 * This function should be called each time a BULK OUT packet is received.
 *
 * It returns a result flag. If the result is ``true``, a packet should be ended. If the result
 * is ``false``, both the BULK OUT and BULK IN endpoint should be stalled.
 */
bool usb_mass_storage_bbb_bulk_out(usb_mass_storage_bbb_state_t *state,
                                   __xdata const uint8_t *data, uint16_t length);

/**
 * Emit USB Mass Storage Bulk-Only Transport interface BULK IN packets.
 *
 * This function should be called each time an IN BULK NAK interrupt occurs.
 *
 * It returns a result flag. If the result is ``true``, a packet should be committed if
 * ``length`` is nonzero. If the result is ``false``, the BULK IN endpoint should be stalled.
 */
bool usb_mass_storage_bbb_bulk_in(usb_mass_storage_bbb_state_t *state,
                                  __xdata uint8_t *data, __xdata uint16_t *length);

#endif
