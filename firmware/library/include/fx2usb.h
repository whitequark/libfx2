#ifndef FX2USB_H
#define FX2USB_H

#include <stdbool.h>
#include <fx2regs.h>
#include <fx2ints.h>
#include <usb.h>

/**
 * Initialize the firmware USB stack. This performs the following:
 *
 *   * enables USB interrupts handled by the support code,
 *   * takes EP0 under software control,
 *   * disconnects (if requested) and connects (if necessary).
 */
void usb_init(bool disconnect);

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
    EP0BCH = tmplength >> 8; \
    EP0BCL = tmplength & 0xff; \
    SUDPTRH = tmpaddr >> 8; \
    SUDPTRL = tmpaddr & 0xff; \
    EP0CS = _HSNAK; \
  } while(0)

/**
 * Configure EP0 for an IN transfer from `EP0BUF`.
 *
 * Using this for an OUT transfer is deprecated, because it exposes a
 * race condition. For OUT transfers please use one or more calls to
 * `SETUP_EP0_OUT_BUF()` instead followed by a single call to `ACK_EP0()`.
 * Do not call `ACK_EP0()` before processing all pending data in `EP0BUF`
 */
#define SETUP_EP0_BUF(length) \
  do { \
    SUDPTRCTL = _SDPAUTO; \
    EP0BCH = 0; \
    EP0BCL = length; \
    EP0CS = _HSNAK; \
  } while(0)

#define SETUP_EP0_OUT_BUF() \
  do { \
    SUDPTRCTL = _SDPAUTO; \
    EP0BCH = 0; \
    EP0BCL = 0; \
  } while(0)

/**
 * Acknowledge an EP0 SETUP or OUT transfer.
 */
#define ACK_EP0() \
  do { EP0CS = _HSNAK; } while(0)

/**
 * Indicate an error in response to a EP0 transfer.
 */
#define STALL_EP0() \
  do { EP0CS = _STALL; EP0CS = _STALL|_HSNAK; } while(0)

/**
 * Return a EPnCS register for given USB endpoint index, or 0
 * if no such endpoint exists.
 */
__xdata volatile uint8_t *EPnCS_for_n(uint8_t n);

typedef __code const char *__code const
  usb_ascii_string_c;

/**
 * An USB configuration descriptor.
 * The USB configuration descriptor is followed by the interface, endpoint, and functional
 * descriptors; these are laid out in the way they should be returned in response to
 * the Get Configuration request.
 */
struct usb_configuration {
  struct usb_desc_configuration desc;
  union usb_config_item {
    usb_desc_generic_c     *generic;
    usb_desc_interface_c   *interface;
    usb_desc_endpoint_c    *endpoint;
  } items[];
};

typedef __code const struct usb_configuration
  usb_configuration_c;

typedef __code const struct usb_configuration *__code const
  usb_configuration_set_c;

/**
 * A group of USB descriptors for a single device.
 */
struct usb_descriptor_set {
  usb_desc_device_c           *device;
  usb_desc_device_qualifier_c *device_qualifier;
  uint8_t                      config_count;
  usb_configuration_set_c     *configs;
  uint8_t                      string_count;
  usb_ascii_string_c          *strings;
};

typedef __code const struct usb_descriptor_set
  usb_descriptor_set_c;

/**
 * Helper function for returning descriptors from a set of C structure definitions.
 * This function relaxes all hardware restrictions on descriptor layout by
 * copying the requested descriptor(s) into the scratch RAM.
 * Sets up an EP0 IN transfer if a descriptor is found, stalls EP0 otherwise.
 */
void usb_serve_descriptor(usb_descriptor_set_c *set,
                          enum usb_descriptor type, uint8_t index);

/**
 * Helper function for resetting the endpoint data toggles for a subset of endpoints defined
 * by the configuration value or interface number and alternate setting, which is necessary when
 * processing a Set Configuration or Set Interface request. This function resets
 * the endpoint toggles for the configuration value `usb_config_value`, and
 * (if `interface == 0xff && alt_setting == 0xff`) all endpoints, or (otherwise)
 * all endpoints assigned to interface with fields
 * `bInterfaceNumber == interface && bAlternateSetting == alt_setting`.
 */
void usb_reset_data_toggles(usb_descriptor_set_c *set, uint8_t interface, uint8_t alt_setting);

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
void handle_usb_get_descriptor(enum usb_descriptor type, uint8_t index);

/**
 * Status variable indicating whether the device is in Address state (if the value is 0)
 * or Configured state (the value corresponds to `bConfigurationValue` field of the
 * descriptor of the selected configuration).
 */
extern uint8_t usb_config_value;

/**
 * Callback for the standard Set Configuration request.
 * This callback has a default implementation that sets `usb_config_value` to `config_value`
 * and returns `true` if `config_value` is 0 or 1, and returns `false` otherwise.
 *
 * The default implementation resets the data toggles using `usb_reset_data_toggles` and
 * the global descriptor set; see `handle_usb_get_descriptor`.
 */
bool handle_usb_set_configuration(uint8_t config_value);

/**
 * Callback for the standard Get Configuration request.
 * This callback has a default implementation that sets up an EP0 IN transfer
 * with value `usb_config_value`.
 */
void handle_usb_get_configuration(void);

/**
 * Callback for the standard Set Interface request.
 * This callback has a default implementation that returns `true` if `alt_setting == 0`,
 * and returns `false` otherwise.
 *
 * The default implementation resets the data toggles using `usb_reset_data_toggles` and
 * the global descriptor set; see `handle_usb_get_descriptor`.
 */
bool handle_usb_set_interface(uint8_t interface, uint8_t alt_setting);

/**
 * Callback for the standard Get Interface request.
 * This callback has a default implementation that sets up an EP0 IN transfer
 * with alternate setting number `0`.
 */
void handle_usb_get_interface(uint8_t interface);

/**
 * Callback for the standard Clear Feature - Endpoint - Endpoint Halt request.
 * This callback has a default implementation that acknowledges the transfer
 * and returns `true`.
 *
 * The data toggle and the stall bit are reset by the interrupt handler
 * if the handler returns `true`.
 */
bool handle_usb_clear_endpoint_halt(uint8_t endpoint);

/**
 * Callback for non-standard setup requests.
 * This callback has a default implementation that stalls EP0.
 */
void handle_usb_setup(__xdata struct usb_req_setup *request);

#endif
