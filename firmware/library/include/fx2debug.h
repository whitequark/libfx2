#ifndef FX2DEBUG_H
#define FX2DEBUG_H

#include <stdint.h>
#include <fx2regs.h>
#include <fx2delay.h>
#include <bits/asmargs.h>

#ifndef DOXYGEN

#define _DEBUG_FN_DIV(a, b) (((a)+(b)/2+1)/(b))

// This implementation not only keeps overhead to absolute minimum, but it also takes great care
// to execute in constant time per bit to minimize clock drift.
#define _DEBUG_FN(retty, name, argty, tx, baud) \
  retty name(argty c) __naked {               \
    c;                                        \
    __asm;                                    \
      inv_baud = _DEBUG_FN_DIV(12000000, baud) \
      overhead = 25                           \
                                              \
      push ar7                                \
      push ar6                                \
      push ar5                                \
      push ar4                                \
      push ar3                                \
      push ar2                                \
                                              \
      mov  r2, _ASM_HASH (00002$-00001$)      \
      mov  r3, _ASM_HASH 0                    \
      mov  r4, dpl                            \
      mov  r5, dpl                            \
                                              \
      mov  dptr, _ASM_HASH _CPUCS             \
      movx a, @dptr                           \
      mov  dptr, _ASM_HASH _DEBUG_FN_DIV((inv_baud>>0)-overhead, 4) \
      jb   acc.4, 00000$ /* _CLKSPD1 */       \
      mov  dptr, _ASM_HASH _DEBUG_FN_DIV((inv_baud>>1)-overhead, 4) \
      jb   acc.3, 00000$ /* _CLKSPD0 */       \
      mov  dptr, _ASM_HASH _DEBUG_FN_DIV((inv_baud>>2)-overhead, 4) \
    00000$:                                   \
      mov  r6, dpl                            \
      mov  r7, dph                            \
                                              \
      mov  a, r3                              \
    00001$:                                   \
      clr  _ASM_REG(tx)           ; 2c        \
        sjmp 00003$               ; 3c        \
    00002$:                                   \
      mov  _ASM_REG(tx), c                    \
        sjmp 00003$                           \
      mov  _ASM_REG(tx), c                    \
        sjmp 00003$                           \
      mov  _ASM_REG(tx), c                    \
        sjmp 00003$                           \
      mov  _ASM_REG(tx), c                    \
        sjmp 00003$                           \
      mov  _ASM_REG(tx), c                    \
        sjmp 00003$                           \
      mov  _ASM_REG(tx), c                    \
        sjmp 00003$                           \
      mov  _ASM_REG(tx), c                    \
        sjmp 00003$                           \
      mov  _ASM_REG(tx), c                    \
        sjmp 00003$                           \
      setb _ASM_REG(tx)                       \
        sjmp 00003$                           \
      sjmp 00004$                             \
                                              \
    00003$:                                   \
      add  a, r2                  ; 1c        \
      mov  r3, a                  ; 1c        \
                                              \
      mov  dpl, r6                ; 2c        \
      mov  dph, r7                ; 2c        \
      lcall _delay_4c                         \
                                              \
      mov  a, r4                  ; 1c        \
      rrc  a                      ; 1c        \
      mov  r4, a                  ; 1c        \
                                              \
      mov  dptr, _ASM_HASH (00001$) ; 3c      \
      mov  a, r3                  ; 1c        \
      jmp  @a+dptr                ; 3c        \
                                              \
    00004$:                                   \
      mov  dpl, r5                            \
                                              \
      pop  ar2                                \
      pop  ar3                                \
      pop  ar4                                \
      pop  ar5                                \
      pop  ar6                                \
      pop  ar7                                \
      _ASM_RET                                \
    __endasm;                                 \
  }

#endif

/**
 * This macro defines a function `void name(uint8_t c)` that implements a robust blocking serial
 * transmitter for debug output. The `tx` parameter may point to any pin, and is defined in
 * the format `Pxn`. The serial format is fixed at 8 data bits, no parity, 1 stop bit, and
 * the baud rate is configurable, up to 230400 at 48 MHz, up to 115200 at 24 MHz, and up to
 * 57600 at 12 MHz.
 *
 * For example, invoking the macro as `DEFINE_DEBUG_FN(tx_byte, PA0, 57600)` defines a routine
 * `void tx_byte(uint8_t c)` that assumes an UART receiver's RX pin is connected to A0.
 */
#define DEFINE_DEBUG_FN(name, tx, baud) \
  _DEBUG_FN(void, name, uint8_t, tx, baud)

/**
 * Same as `DEFINE_DEBUG_FN()`, but defines an `int putchar(int c)` routine that can be used with
 * the `printf` family of functions. (The C standard requires `putchar` to have the specified
 * signature).
 */
#if (__SDCC_VERSION_MAJOR > 3) || ((__SDCC_VERSION_MAJOR == 3) && (__SDCC_VERSION_MINOR > 6))
#define DEFINE_DEBUG_PUTCHAR_FN(tx, baud) \
  _DEBUG_FN(int, putchar, int, tx, baud)
#else
#define DEFINE_DEBUG_PUTCHAR_FN(tx, baud) \
  _DEBUG_FN(void, putchar, char, tx, baud)
#endif

#endif
