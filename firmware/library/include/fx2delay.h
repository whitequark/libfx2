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
#define _NOP2     __asm__("nop \n nop")
#define _NOP3     __asm__("nop \n nop \n nop")
#define _NOPn(n)  __asm__("lcall _nop" #n)
#endif

/**
 * Synchronization delay for access to certain registers.
 *
 * See TRM 15.15 for details.
 *
 * This macro produces very compact code, using only 2 or 3 bytes per instance.
 * This comes with a tradeoff that `SYNCDELAYLEN` values between 4 and 7
 * result in a 8 cycle delay.
 */
#if SYNCDELAYLEN == 2
#define SYNCDELAY _NOP2
#elif SYNCDELAYLEN == 3
#define SYNCDELAY _NOP3
#elif SYNCDELAYLEN == 4
#define SYNCDELAY _NOPn(8) // lcall 4c ret 4c
#elif SYNCDELAYLEN == 5
#define SYNCDELAY _NOPn(8) // lcall 4c ret 4c
#elif SYNCDELAYLEN == 6
#define SYNCDELAY _NOPn(8) // lcall 4c ret 4c
#elif SYNCDELAYLEN == 7
#define SYNCDELAY _NOPn(8) // lcall 4c ret 4c
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
