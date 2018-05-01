#ifndef FX2DELAY_H
#define FX2DELAY_H

#include <stdint.h>

/**
 * Spin for the given number of milliseconds.
 */
void delay_ms(uint16_t count);

/**
 * Spin for the given number of microseconds.
 * `count` must be no greater than 21845.
 */
void delay_us(uint16_t count);

/**
 * Spin for `count * 4` processor cycles, or `count * 16` clock cycles.
 * Takes exactly 28 processor cycles if `count` is less than that.
 */
void delay_4c(uint16_t count);

/**
 * Synchronization delay length.
 * This value defaults to 3, and should be overridden using a compiler flag
 * `-DSYNCDELAYLEN=n` if running with non-default IFCLK or CLKOUT clock frequency.
 * Delay length can be calculated using the following Python code:
 *
 *     import math
 *     math.ceil(1.5 * ((ifclk_period / clkout_period) + 1))
 *
 * See TRM 15.15 for details.
 */
#ifndef SYNCDELAYLEN
#define SYNCDELAYLEN 3
#endif

#if SYNCDELAYLEN < 2 || SYNCDELAYLEN > 16
#error Invalid synchronization delay length
#endif

#ifndef DOXYGEN
#define _NOP __asm__("nop")
#endif

/**
 * Synchronization delay for access to certin registers.
 * See TRM 15.15 for details.
 */
#if SYNCDELAYLEN == 2
#define SYNCDELAY() \
  do { _NOP; _NOP; } while(0)
#elif SYNCDELAYLEN == 3
#define SYNCDELAY() \
  do { _NOP; _NOP; _NOP; } while(0)
#elif SYNCDELAYLEN == 4
#define SYNCDELAY() \
  do { _NOP; _NOP; _NOP; _NOP; } while(0)
#elif SYNCDELAYLEN == 5
#define SYNCDELAY() \
  do { _NOP; _NOP; _NOP; _NOP; _NOP; } while(0)
#elif SYNCDELAYLEN == 6
#define SYNCDELAY() \
  do { _NOP; _NOP; _NOP; _NOP; _NOP; _NOP; } while(0)
#elif SYNCDELAYLEN == 7
#define SYNCDELAY() \
  do { _NOP; _NOP; _NOP; _NOP; _NOP; _NOP; _NOP; } while(0)
#elif SYNCDELAYLEN == 8
#define SYNCDELAY() \
  do { _NOP; _NOP; _NOP; _NOP; _NOP; _NOP; _NOP; _NOP; } while(0)
#elif SYNCDELAYLEN == 9
#define SYNCDELAY() \
  do { _NOP; _NOP; _NOP; _NOP; _NOP; _NOP; _NOP; _NOP; \
       _NOP; } while(0)
#elif SYNCDELAYLEN == 10
#define SYNCDELAY() \
  do { _NOP; _NOP; _NOP; _NOP; _NOP; _NOP; _NOP; _NOP; \
       _NOP; _NOP; } while(0)
#elif SYNCDELAYLEN == 11
#define SYNCDELAY() \
  do { _NOP; _NOP; _NOP; _NOP; _NOP; _NOP; _NOP; _NOP; \
       _NOP; _NOP; _NOP; } while(0)
#elif SYNCDELAYLEN == 12
#define SYNCDELAY() \
  do { _NOP; _NOP; _NOP; _NOP; _NOP; _NOP; _NOP; _NOP; \
       _NOP; _NOP; _NOP; _NOP; } while(0)
#elif SYNCDELAYLEN == 13
#define SYNCDELAY() \
  do { _NOP; _NOP; _NOP; _NOP; _NOP; _NOP; _NOP; _NOP; \
       _NOP; _NOP; _NOP; _NOP; _NOP; } while(0)
#elif SYNCDELAYLEN == 14
#define SYNCDELAY() \
  do { _NOP; _NOP; _NOP; _NOP; _NOP; _NOP; _NOP; _NOP; \
       _NOP; _NOP; _NOP; _NOP; _NOP; _NOP; } while(0)
#elif SYNCDELAYLEN == 15
#define SYNCDELAY() \
  do { _NOP; _NOP; _NOP; _NOP; _NOP; _NOP; _NOP; _NOP; \
       _NOP; _NOP; _NOP; _NOP; _NOP; _NOP; _NOP; } while(0)
#elif SYNCDELAYLEN == 16
#define SYNCDELAY() \
  do { _NOP; _NOP; _NOP; _NOP; _NOP; _NOP; _NOP; _NOP; \
       _NOP; _NOP; _NOP; _NOP; _NOP; _NOP; _NOP; _NOP; } while(0)
#endif

#endif
