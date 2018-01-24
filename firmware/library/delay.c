#include <fx2delay.h>
#include <fx2regs.h>

// Spin for exactly min(28, DP*4) cycles (i.e. min(112, DP*16) clocks).
// The implementation is a bit complicated, but the calculations
// in the callee are simpler.
void delay_4c(uint16_t delay) __naked {
  delay;
  __asm
    // subtract fixed delay; 13c prolog, 11c loop conditions, 4c epilog
    clr  c            ; 1c
    mov  a, dpl       ; 2c
    subb a, #7        ; 2c
    mov  dpl, a       ; 2c
    mov  a, dph       ; 2c
    subb a, #0        ; 2c
    mov  dph, a       ; 2c

    // only run for minimum cycle count on underflow
    jnc  00000$       ; 3c
    // we've ran for 16 cycles, but need 28, fill in
    mov  dpl, dpl     ; 3c nop
    mov  dpl, dpl     ; 3c nop
    mov  a, dpl       ; 2c nop
    ret               ; 4c

  00000$:
    // don't run the DPH loop if DPH is zero
    jz   00003$       ; 3c
    // loop for DPH*256*4 cycles, DPH*512 instructions
  00001$:
      mov  a, #0xfe     ; 2c
      nop               ; 1c
      nop               ; 1c
  00002$:
        dec  a            ; 1c
        jnz  00002$       ; 3c
      djnz dph, 00001$  ; 4c

  00003$:
    // don't run the DPL loop if DPL is zero
    mov  a, dpl       ; 2c
    jz   00005$       ; 3c
    // loop for DPL*4 cycles, DPL instructions
  00004$:
      djnz dpl, 00004$  ; 4c

  00005$:
    ret               ; 4c
  __endasm;
}

void delay_us(uint16_t count) {
  // Empirically correct for our CLKSPD detection code.
#define OVERHEAD 20
  // 1 loop iteration is 16 clocks.
  // At 48 MHz, 16 clocks is 1/3 µs.
  // Thus, iteration-count = µs-count * 3.
  // At 24 and 12 MHz we divide that by 2 and 4.
  uint16_t iters = count + count + count;
  uint8_t cpucs = CPUCS;
  if(cpucs & _CLKSPD1)
    ;
  else if(cpucs & _CLKSPD0)
    iters >>= 1;
  else
    iters >>= 2;
  if(iters <= OVERHEAD)
    return;
  iters = iters - OVERHEAD;
  delay_4c(iters);
}
#undef OVERHEAD

void delay_ms(uint16_t count) {
  // Empirically correct for our CLKSPD detection code.
#define OVERHEAD 5
  // Inlined version of delay_us above.
  uint16_t iters;
  uint8_t cpucs = CPUCS;
  if(cpucs & _CLKSPD1)
    iters = 1000 * 3 - OVERHEAD;
  else if(cpucs & _CLKSPD0)
    iters = 1000 * 3 / 2 - OVERHEAD;
  else
    iters = 1000 * 3 / 4 - OVERHEAD;
  while(count--)
    delay_4c(iters);
}
#undef OVERHEAD
