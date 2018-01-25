#ifndef FX2EEPROM_H
#define FX2EEPROM_H

#include <stdint.h>
#include <stdbool.h>

/**
 * This function reads `len` bytes at memory address `addr` from EEPROM chip
 * with bus address `chip` to `buf`.
 * It writes two address bytes if `double_byte` is true.
 */
bool eeprom_read(uint8_t chip, uint16_t addr, uint8_t *buf, uint16_t len, bool double_byte);

/**
 * This function writes `len` bytes at memory address `addr` to EEPROM chip
 * with bus address `chip` from `buf`.
 * It writes two address bytes if `double_byte` is true.
 */
bool eeprom_write(uint8_t chip, uint16_t addr, uint8_t *buf, uint16_t len, bool double_byte);

#endif
