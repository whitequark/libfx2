#ifndef FX2LIB_H
#define FX2LIB_H

#include <stdint.h>

/**
 * 0.5KB of general purpose scratch RAM.
 */
__xdata __at(0xe000) uint8_t scratch[512];

/**
 * A fast memory copy routine that uses the FX2-specific architecture extensions.
 * This routine clobbers the value of all autopointer registers.
 * Both pointers should be in the `__xdata` address space.
 * Returns destination, like `memcpy`.
 */
__xdata void *xmemcpy(__xdata void *dest, __xdata void *src, uint16_t length);

#endif
