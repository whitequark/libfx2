#ifndef FX2SPIFLASH_H
#define FX2SPIFLASH_H

#include <fx2spi.h>

#ifndef DOXYGEN

#define _DEFINE_SPIFLASH_STORAGE(name)                                            \
  __xdata uint8_t _##name##_spiflash_buf[4];

#define _DEFINE_SPIFLASH_INIT_FN(name, cs, sck, si, so)                           \
  void name##_init() {                                                            \
    __asm setb cs  __endasm;                                                      \
    __asm setb sck __endasm;                                                      \
    __asm setb si  __endasm;                                                      \
  }

#define _DEFINE_SPIFLASH_RDP_FN(name, cs)                                         \
  void name##_rdp() {                                                             \
    _##name##_spiflash_buf[0] = 0xAB;                                             \
    __asm clr  cs  __endasm;                                                      \
    _##name##_spi_wr(_##name##_spiflash_buf, 1);                                  \
    __asm setb cs  __endasm;                                                      \
  }

#define _DEFINE_SPIFLASH_DP_FN(name, cs)                                          \
  void name##_dp() {                                                              \
    _##name##_spiflash_buf[0] = 0xB9;                                             \
    __asm clr  cs  __endasm;                                                      \
    _##name##_spi_wr(_##name##_spiflash_buf, 1);                                  \
    __asm setb cs  __endasm;                                                      \
  }

#define _DEFINE_SPIFLASH_READ_FN(name, cs)                                        \
  void name##_read(uint32_t addr, __xdata uint8_t *data, uint16_t length) {       \
    _##name##_spiflash_buf[0] = 0x03;                                             \
    _##name##_spiflash_buf[1] = (addr >> 16);                                     \
    _##name##_spiflash_buf[2] = (addr >> 8);                                      \
    _##name##_spiflash_buf[3] = (addr >> 0);                                      \
    __asm clr  cs  __endasm;                                                      \
    _##name##_spi_wr(_##name##_spiflash_buf, 4);                                  \
    _##name##_spi_rd(data, length);                                               \
    __asm setb cs  __endasm;                                                      \
  }

#define _DEFINE_SPIFLASH_WREN_FN(name, cs)                                        \
  void name##_wren() {                                                            \
    _##name##_spiflash_buf[0] = 0x06;                                             \
    __asm clr  cs  __endasm;                                                      \
    _##name##_spi_wr(_##name##_spiflash_buf, 1);                                  \
    __asm setb cs  __endasm;                                                      \
  }

#define _DEFINE_SPIFLASH_RDSR_FN(name, cs)                                        \
  uint8_t name##_rdsr() {                                                         \
    _##name##_spiflash_buf[0] = 0x05;                                             \
    __asm clr  cs  __endasm;                                                      \
    _##name##_spi_wr(_##name##_spiflash_buf, 1);                                  \
    _##name##_spi_rd(_##name##_spiflash_buf, 1);                                  \
    __asm setb cs  __endasm;                                                      \
    return _##name##_spiflash_buf[0];                                             \
  }

#define _DEFINE_SPIFLASH_CE_FN(name, cs)                                          \
  void name##_ce() {                                                              \
    _##name##_spiflash_buf[0] = 0x60;                                             \
    __asm clr  cs  __endasm;                                                      \
    _##name##_spi_wr(_##name##_spiflash_buf, 1);                                  \
    __asm setb cs  __endasm;                                                      \
  }

#define _DEFINE_SPIFLASH_SE_FN(name, cs)                                          \
  void name##_se(uint32_t addr) {                                                 \
    _##name##_spiflash_buf[0] = 0x20;                                             \
    _##name##_spiflash_buf[1] = (addr >> 16);                                     \
    _##name##_spiflash_buf[2] = (addr >> 8);                                      \
    _##name##_spiflash_buf[3] = (addr >> 0);                                      \
    __asm clr  cs  __endasm;                                                      \
    _##name##_spi_wr(_##name##_spiflash_buf, 4);                                  \
    __asm setb cs  __endasm;                                                      \
  }

#define _DEFINE_SPIFLASH_PP_FN(name, cs)                                          \
  void name##_pp(uint32_t addr, const __xdata uint8_t *data, uint16_t length) {   \
    _##name##_spiflash_buf[0] = 0x02;                                             \
    _##name##_spiflash_buf[1] = (addr >> 16);                                     \
    _##name##_spiflash_buf[2] = (addr >> 8);                                      \
    _##name##_spiflash_buf[3] = (addr >> 0);                                      \
    __asm clr  cs  __endasm;                                                      \
    _##name##_spi_wr(_##name##_spiflash_buf, 4);                                  \
    _##name##_spi_wr(data, length);                                               \
    __asm setb cs  __endasm;                                                      \
  }

#endif

/// Write-in-Progress status register bit.
#define SPIFLASH_WIP 0b00000001

/// Write Enable Latch status register bit.
#define SPIFLASH_WEL 0b00000010

/**
 * This macro defines a number of functions that implement common operations on 25C-compatible
 * SPI flashes. They are optimized and run at ~5 MHz SCK frequency at 48 MHz CLKOUT.
 * The `cs`, `sck`, `so`, and `si` parameters may point to any pins, and are defined in the
 * format `_IOx+n` (note the underscore).
 *
 * The defined routines are:
 *
 *   * `void name_init()`, to set outputs to their idle state. (Note that output enables must be
 *     configured by the user before calling `name_init`.)
 *   * `void name_rdp()`, to leave deep power-down mode (command `AB`).
 *   * `void name_dp()`, to enter deep power-down mode (command `B9`).
 *   * `void name_read(uint32_t addr, __xdata uint8_t *data, uint16_t length)`, to read data at
 *     the given address, with wraparound at array boundary (command `03`).
 *   * `void name_wren()`, to latch the write enable bit (command `06`).
 *   * `uint8_t name_rdsr()`, to read the status register (command `05`).
 *   * `void name_ce()`, to erase the entire chip (command `60`).
 *   * `void name_se(uint32_t addr)`, to erase a sector at the given address (command `20`).
 *   * `void name_pp(uint32_t addr, const __xdata uint8_t *data, uint16_t length)`, to program
 *     up to a whole page at the given address, with wraparound at page boundary (command `02`).
 *
 * For example, invoking the macro as `DEFINE_SPIFLASH_FNS(flash, _IOA+0, _IOB+0, _IOB+1, _IOB+2)`
 * defines the routines `void flash_init()`, `void flash_read()`, etc that assume an SPI flash's
 * CS# pin is connected to A0, SCK pin is connected to B0, MISO pin is connected to B1, and MOSI
 * pin is connected to B2.
 */
#define DEFINE_SPIFLASH_FNS(name, cs, sck, si, so)  \
  DEFINE_SPI_WR_FN(_##name##_spi_wr, sck, si)       \
  DEFINE_SPI_RD_FN(_##name##_spi_rd, sck, so)       \
  _DEFINE_SPIFLASH_STORAGE(name)                    \
  _DEFINE_SPIFLASH_INIT_FN(name, cs, sck, si, so)   \
  _DEFINE_SPIFLASH_RDP_FN (name, cs)                \
  _DEFINE_SPIFLASH_DP_FN  (name, cs)                \
  _DEFINE_SPIFLASH_READ_FN(name, cs)                \
  _DEFINE_SPIFLASH_WREN_FN(name, cs)                \
  _DEFINE_SPIFLASH_RDSR_FN(name, cs)                \
  _DEFINE_SPIFLASH_CE_FN  (name, cs)                \
  _DEFINE_SPIFLASH_SE_FN  (name, cs)                \
  _DEFINE_SPIFLASH_PP_FN  (name, cs)

#endif
