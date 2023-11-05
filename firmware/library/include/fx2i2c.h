#ifndef FX2I2C_H
#define FX2I2C_H

#if !defined(__SDCC_MODEL_HUGE)
#pragma callee_saves i2c_wait
#pragma callee_saves i2c_start
#pragma callee_saves i2c_stop
#pragma callee_saves i2c_write
#pragma callee_saves i2c_read
#endif

#include <stdint.h>
#include <stdbool.h>

/**
 * A flag that terminates the current I2C transfer.
 */
extern volatile bool i2c_cancel;

/**
 * This function waits until the current I2C transfer terminates,
 * or until `i2c_cancel` is set. In the latter case, `i2c_cancel` is cleared.
 * Returns `false` in case of bus contention, or if `need_ack` was set
 * and the transfer was not acknowledged, `true` otherwise.
 * The `BERR` bit in the `I2CS` register should be checked if `false`
 * is returned.
 *
 * Note that `i2c_wait` is not meant to be used directly; it is documented
 * to explain the behavior of `i2c_start`, `i2c_write` and `i2c_read`.
 */
bool i2c_wait(bool need_ack);

/**
 * This function generates a start condition and writes the address `chip`
 * to the bus. See `i2c_wait` for description of return value.
 */
bool i2c_start(uint8_t chip);

/**
 * This function generates a stop condition.
 */
bool i2c_stop(void);

/**
 * This function writes `len` bytes from `buf` to the I2C bus.
 * See `i2c_wait` for description of return value.
 */
bool i2c_write(const uint8_t *buf, uint16_t len);

/**
 * This function reads `len` bytes to `buf` from the I2C bus, responds with NAK
 * to the last byte read, and generates a stop condition.
 * See `i2c_wait` for description of return value.
 */
bool i2c_read(uint8_t *buf, uint16_t len);

#endif
