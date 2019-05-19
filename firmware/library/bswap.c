#include <fx2lib.h>
#include <fx2regs.h>

uint16_t bswap16(uint16_t value) __naked {
  value;
__asm;
  xch  a, dpl   ; 2c
  xch  a, dph   ; 2c
  xch  a, dpl   ; 2c
#if !defined(__SDCC_MODEL_HUGE)
  ret           ; 4c
#else
  ljmp  __sdcc_banked_ret
#endif
__endasm;
}

uint32_t bswap32(uint32_t value) __naked {
  value;
__asm;
  xch  a, dpl   ; 2c
  xch  a, b     ; 2c
  xch  a, dph   ; 2c
  xch  a, b     ; 2c
#if !defined(__SDCC_MODEL_HUGE)
  ret           ; 4c
#else
  ljmp  __sdcc_banked_ret
#endif
__endasm;
}
