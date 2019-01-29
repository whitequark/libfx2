#ifndef FX2SPI_H
#define FX2SPI_H

#include <stdint.h>
#include <fx2regs.h>
#include <bits/asmargs.h>

#ifndef DOXYGEN

// See the implementation of xmemcpy for a detailed explanation of the loop structure below.
#define _SPI_FN(name, cst, bit, sck, si, so, aldr, atlr)   \
  void name(cst __xdata uint8_t *data, uint16_t len) {    \
    data;                                             \
    len;                                              \
    __asm                                             \
      mov  _AUTOPTRSETUP, _ASM_HASH 0b11              \
      mov  _AUTOPTRL1, dpl                            \
      mov  _AUTOPTRH1, dph                            \
                                                      \
      _ASM_GET_PARM(r2, r3, _##name##_PARM_2)         \
                                                      \
      mov  a, r2                                      \
      jz   00000$                                     \
      inc  r3                                         \
    00000$:                                           \
      mov  a, r3                                      \
      jz   00002$                                     \
                                                      \
      mov  dptr, _ASM_HASH _XAUTODAT1                 \
    00001$:                                           \
        aldr                   ; 8c+s                 \
        bit(sck, si, so, 7)    ; 8c                   \
        bit(sck, si, so, 6)    ; 8c                   \
        bit(sck, si, so, 5)    ; 8c                   \
        bit(sck, si, so, 4)    ; 8c                   \
        bit(sck, si, so, 3)    ; 8c                   \
        bit(sck, si, so, 2)    ; 8c                   \
        bit(sck, si, so, 1)    ; 8c                   \
        bit(sck, si, so, 0)    ; 8c                   \
        atlr                   ; 8c+s                 \
        djnz r2, 00001$        ; 4c                   \
      djnz r3, 00001$        ; 4c                     \
                                                      \
    00002$:                                           \
    __endasm;                                         \
  }

#define _SPI_DUMMY

#define _SPI_WR_LDR movx a, @dptr
#define _SPI_WR_BIT(sck, si, so, num) \
  mov  c, acc+num ; 2c                \
  clr  sck        ; 2c                \
  mov  si, c      ; 2c                \
  setb sck        ; 2c

#define _SPI_RD_TLR movx @dptr, a
#define _SPI_RD_BIT(sck, si, so, num) \
  clr  sck        ; 2c                \
  mov  c, so      ; 2c                \
  setb sck        ; 2c                \
  mov  acc+num, c ; 2c

#endif

/**
 * This macro defines a function `void name(const __xdata uint8_t *data, uint16_t len)` that
 * implements an optimized (76 clock cycles per iteration; ~5 MHz at 48 MHz CLKOUT) SPI Mode 3
 * write routine.
 * The `sck` and `si` parameters may point to any pins, and are defined in the format `_Pxn`
 * (note the underscore).
 *
 * For example, invoking the macro as `DEFINE_SPI_WR_FN(flash_write, _PA1, _PB6)` defines
 * a routine `void flash_write()` that assumes an SPI device's SCK pin is connected to A1 and
 * MOSI pin is connected to B6.
 */
#define DEFINE_SPI_WR_FN(name, sck, si) \
  _SPI_FN(name, const, _SPI_WR_BIT, sck, si, 0, _SPI_WR_LDR, _SPI_DUMMY)

/**
 * This macro defines a function `void name(__xdata uint8_t *data, uint16_t len)` that implements
 * an optimized (76 clock cycles per iteration; ~5 MHz at 48 MHz CLKOUT) SPI Mode 3 read routine.
 * The `sck` and `so` parameters may point to any pins, and are defined in the format `_Pxn`
 * (note the underscore).
 *
 * For example, invoking the macro as `DEFINE_SPI_RD_FN(flash_read, _PA1, _PB5)` defines
 * a routine `void flash_read()` that assumes an SPI device's SCK pin is connected to A1 and
 * MISO pin is connected to B5.
 */
#define DEFINE_SPI_RD_FN(name, sck, so) \
  _SPI_FN(name, _SPI_DUMMY, _SPI_RD_BIT, sck, 0, so, _SPI_DUMMY, _SPI_RD_TLR)

#endif
