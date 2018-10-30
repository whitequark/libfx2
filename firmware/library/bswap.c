#include <fx2lib.h>
#include <fx2regs.h>

uint16_t bswap16(uint16_t value) __naked {
  value;
__asm;
  mov  a, dph
  mov  dpl, dph
  mov  dpl, a
#if defined(__SDCC_MODEL_HUGE)
  ljmp  __sdcc_banked_ret
#else
  ret
#endif
__endasm;
}

uint32_t bswap32(uint32_t value) __naked {
  value;
__asm;
#if defined(__SDCC_MODEL_SMALL)
  ar0 = 0x00
  push ar0
#endif
  mov  r0, a
  mov  a, dpl
  mov  dpl, r0
  mov  r0, b
  mov  b, dph
  mov  dph, r0
#if defined(__SDCC_MODEL_SMALL)
  pop  ar0
#endif
#if defined(__SDCC_MODEL_HUGE)
  ljmp  __sdcc_banked_ret
#else
  ret
#endif
__endasm;
}
