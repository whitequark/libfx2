#ifndef FX2USB_H
#define FX2USB_H

#include <stdbool.h>
#include <usb.h>
#include <fx2regs.h>
#include <fx2ints.h>

/**
 * Initialize the firmware USB stack. This performs the following:
 *
 *   * enables USB interrupts handled by the support code,
 *   * takes EP0 under software control,
 *   * disconnects (if requested) and connects (if necessary).
 */
void usb_init(bool reconnect);

/**
 * Configure EP0 for an IN transfer from RAM of an USB descriptor
 * of the following types:
 *
 *   * Device
 *   * Device Qualifier
 *   * Configuration
 *   * Other Speed Configuration
 *   * String
 *
 * Note that `addr` must be word-aligned.
 */
#define SETUP_EP0_IN_DESC(addr) \
  do { \
    uint16_t tmp = (uint16_t)(addr); \
    SUDPTRCTL = _SDPAUTO; \
    SUDPTRH = tmp >> 8; \
    SUDPTRL = tmp & 0xff; \
    EP0CS = _HSNAK; \
  } while(0)

/**
 * Configure EP0 for an IN transfer from RAM of data of arbitrary length.
 * Note that `addr` must be word-aligned.
 */
#define SETUP_EP0_IN_DATA(addr, length) \
  do { \
    uint16_t tmpaddr = (uint16_t)(addr), \
             tmplength = (uint16_t)(length); \
    SUDPTRCTL = 0; \
    EP0BCH = length >> 8; \
    EP0BCL = length & 0xff; \
    SUDPTRH = tmpaddr >> 8; \
    SUDPTRL = tmpaddr & 0xff; \
    EP0CS = _HSNAK; \
  } while(0)

/**
 * Configure EP0 for an IN or OUT transfer from or to `EP0BUF`.
 * For an OUT transfer, specify `length` as `0`.
 */
#define SETUP_EP0_BUF(length) \
  do { \
    SUDPTRCTL = _SDPAUTO; \
    EP0BCH = 0; \
    EP0BCL = length; \
    EP0CS = _HSNAK; \
  } while(0)

/**
 * Acknowledge an EP0 OUT transfer.
 */
#define SETUP_EP0_ACK() \
  do { EP0CS = _HSNAK; } while(0)

/**
 * Indicate an error in response to a EP0 transfer.
 */
#define STALL_EP0() \
  do { EP0CS = _STALL; } while(0)

/**
 * Return a EPnCS register for given USB endpoint index, or 0
 * if no such endpoint exists.
 */
__xdata volatile uint8_t *EPnCS_for_n(uint8_t n);

/**
 * A group of USB descriptors for a single device.
 * The interface and endpoint descriptors are laid out linearly,
 * e.g. if configuration 0 has interfaces 0(a), and configuration 1 has
 * interfaces 0(b) and 1(b), the `interfaces` array would contain 0(a), 0(b), 1(b).
 */
struct usb_descriptor_set {
  const struct usb_desc_device        *device;
  uint8_t                              config_count;
  const struct usb_desc_configuration *configs;
  uint8_t                              interface_count;
  const struct usb_desc_interface     *interfaces;
  uint8_t                              endpoint_count;
  const struct usb_desc_endpoint      *endpoints;
  uint8_t                              string_count;
  const char *                  const *strings;
};

/**
 * Helper function for returning descriptors from a set of C structure definitions.
 * This function relaxes all hardware restrictions on descriptor layout by
 * copying the requested descriptor(s) into the scratch RAM.
 * Returns `true` and sets up an EP0 IN transfer if a descriptor is found,
 * returns `false` otherwise.
 */
bool usb_serve_descriptor(const struct usb_descriptor_set *set,
                          enum usb_descriptor type, uint8_t index);

/**
 * Status variable indicating whether the device is currently self-powered.
 * The value of this variable is returned via the standard Get Status - Device request.
 */
extern bool usb_self_powered;

/**
 * Status variable indicating whether the device is configured for remote wakeup.
 * The value of this variable is returned via the standard Get Status - Device request.
 */
extern bool usb_remote_wakeup;

/**
 * Callback for the standard Get Descriptor request.
 * This callback has a default implementation that returns descriptors
 * from a global `const struct usb_descriptor_set usb_descriptor_set = { ... };`.
 * See `usb_serve_descriptor`.
 */
bool handle_usb_get_descriptor(enum usb_descriptor type, uint8_t index);

/**
 * Callback for the standard Set Configuration request.
 * This callback has a default implementation that returns `true`
 * and acknowledges the transfer if `value == 0` and `false` otherwise.
 */
bool handle_usb_set_configuration(uint8_t value);

/**
 * Callback for the standard Get Configuration request.
 * This callback has a default implementation that returns `true` and
 * sets up an EP0 IN transfer with configuration value `0`.
 */
bool handle_usb_get_configuration();

/**
 * Callback for the standard Set Interface request.
 * This callback has a default implementation that returns `true`
 * and acknowledges the transfer if `value == 0` and `false` otherwise.
 */
bool handle_usb_set_interface(uint8_t index, uint8_t value);

/**
 * Callback for the standard Get Interface request.
 * This callback has a default implementation that returns `true` and
 * sets up an EP0 IN transfer with alternate setting number `0`.
 */
bool handle_usb_get_interface(uint8_t index);

/**
 * Callback for the standard Clear Feature - Endpoint - Endpoint Halt request.
 * This callback has a default implementation that returns `true`
 * and acknowledges the transfer.
 * The data toggle and the stall bit are reset by the interrupt handler
 * if the callback returns `true`.
 */
bool handle_usb_clear_endpoint_halt(uint8_t index);

/**
 * Callback for non-standard setup requests.
 * This callback has a default implementation that returns `false`.
 */
bool handle_usb_request(__xdata struct usb_req_setup *request);

#endif
