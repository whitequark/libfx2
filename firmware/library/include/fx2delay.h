#ifndef FX2DELAY_H
#define FX2DELAY_H

#if !defined(__SDCC_MODEL_HUGE)
#pragma callee_saves delay_ms
#pragma callee_saves delay_us
#pragma callee_saves delay_4c
#endif

#include <stdint.h>

/**
 * Spin for the given number of milliseconds.
 */
void delay_ms(uint16_t count_ms);

/**
 * Spin for the given number of microseconds, minus `overh_c` processor cycles.
 * `count_us` must be no greater than 21845, and `overh_c` must be no greater than 128.
 *
 * This function is cycle-accurate at any CPU clock frequency provided that the delay is not less
 * than the intrinsic overhead of up to 100 processor cycles (9..33 microseconds).
 */
void delay_us_overhead(uint16_t count_us, uint8_t overh_c) __reentrant;

/**
 * Equivalent to `delay_us_overhead(count_us, 3)` where 3 is the number of cycles of overhead
 * when `delay_us` is called with a constant argument.
 */
void delay_us(uint16_t count_us);

/**
 * Spin for `count * 4` processor cycles, or `count * 16` clock cycles.
 * Takes exactly 32 processor cycles (3..10 microseconds) if `count` is less than `8`.
 */
void delay_4c(uint16_t count_4c);

/**
 * Synchronization delay length.
 *
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
// Accumulator doesn't need to be preserved
#define _NOP1     __asm__("nop")                            // 1b 1c
#define _NOP2     __asm__("nop  \n nop")                    // 2b 2c
#define _NOP3     __asm__("ajmp  .+2")                      // 2b 3c
#define _NOP4     __asm__("nop          \n movc a, @a+pc")  // 2b 4c
#define _NOP5     __asm__("nop  \n nop  \n movc a, @a+pc")  // 3b 5c
#define _NOP6     __asm__("ajmp  .+2    \n movc a, @a+pc")  // 3b 6c
#define _NOP7     __asm__("acall .+2    \n ret")            // 3b 7c
#define _NOPn(n)  __asm__("lcall _nop"#n)                   // 3b nc 8<=n<=16
#endif

/**
 * Synchronization delay for access to certain registers.
 *
 * See TRM 15.15 for details.
 *
 * This macro produces very compact code, using only 2 or 3 bytes per instance.
 */
#if SYNCDELAYLEN == 2
#define SYNCDELAY _NOP2
#elif SYNCDELAYLEN == 3
#define SYNCDELAY _NOP3
#elif SYNCDELAYLEN == 4
#define SYNCDELAY _NOP4
#elif SYNCDELAYLEN == 5
#define SYNCDELAY _NOP5
#elif SYNCDELAYLEN == 6
#define SYNCDELAY _NOP6
#elif SYNCDELAYLEN == 7
#define SYNCDELAY _NOP7
#elif SYNCDELAYLEN == 8
#define SYNCDELAY _NOPn(8)
#elif SYNCDELAYLEN == 9
#define SYNCDELAY _NOPn(9)
#elif SYNCDELAYLEN == 10
#define SYNCDELAY _NOPn(10)
#elif SYNCDELAYLEN == 11
#define SYNCDELAY _NOPn(11)
#elif SYNCDELAYLEN == 12
#define SYNCDELAY _NOPn(12)
#elif SYNCDELAYLEN == 13
#define SYNCDELAY _NOPn(13)
#elif SYNCDELAYLEN == 14
#define SYNCDELAY _NOPn(14)
#elif SYNCDELAYLEN == 15
#define SYNCDELAY _NOPn(15)
#elif SYNCDELAYLEN == 16
#define SYNCDELAY _NOPn(16)
#endif

#endif
