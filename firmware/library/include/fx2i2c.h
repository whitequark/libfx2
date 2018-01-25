#ifndef FX2I2C_H
#define FX2I2C_H

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
 */
bool i2c_wait(bool need_ack);

/**
 * This function generates a start condition and writes the address `chip`
 * to the bus.
 */
bool i2c_start(uint8_t chip);

/**
 * This function generates a stop condition.
 */
void i2c_stop();

/**
 * This function writes `len` bytes from `buf` to the I2C bus.
 */
bool i2c_write(uint8_t *buf, uint16_t len);

/**
 * This function reads `len` bytes to `buf` from the I2C bus, responds with NAK
 * to the last byte read, and generates a stop condition.
 */
bool i2c_read(uint8_t *buf, uint16_t len);

#endif
