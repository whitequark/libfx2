#ifndef FX2EEPROM_H
#define FX2EEPROM_H

#include <stdint.h>
#include <stdbool.h>

/**
 * This function reads `len` bytes at memory address `addr` from EEPROM chip
 * with bus address `chip` to `buf`. It writes two address bytes if `double_byte` is true.
 * Returns `true` if the read is successful, `false` otherwise.
 */
bool eeprom_read(uint8_t chip, uint16_t addr, uint8_t *buf, uint16_t len, bool double_byte);

/**
 * This function writes `len` bytes at memory address `addr` to EEPROM chip
 * with bus address `chip` from `buf`. It writes two address bytes if `double_byte` is true.
 * `timeout` specifies the number of polling attempts after a write; 0 means wait forever.
 * Returns `true` if the write is successful, `false` otherwise.
 *
 * At 100 kHz, one polling attempt is ~120 us, at 400 kHz, ~30 us, so for
 * typical wait cycles of up to 5 ms, a timeout of 166 should be sufficient in all cases.
 */
bool eeprom_write(uint8_t chip, uint16_t addr, uint8_t *buf, uint16_t len, bool double_byte,
                  uint8_t timeout);

#endif
