#include <fx2delay.h>
#include <fx2regs.h>

// Spin for exactly min(32, DP*4) cycles (i.e. min(128, DP*16) clocks), including the call of
// this function. The implementation is a bit complicated, but the calculations in the callee
// are simpler.
void delay_4c(uint16_t count) __naked {
  count;
  __asm
    ; (ljmp delay_4c) ; 4c
    // subtract fixed delay; 4c ljmp, 13c prolog, 11c loop conditions, 4c epilog
    clr  c            ; 1c
    mov  a, dpl       ; 2c
    subb a, #8        ; 2c
    mov  dpl, a       ; 2c
    mov  a, dph       ; 2c
    subb a, #0        ; 2c
    mov  dph, a       ; 2c

    // only run for minimum cycle count on underflow
    jnc  00000$       ; 3c
    // we've ran for 20 cycles, but need 32, fill in
    nop               ; 1c
    nop               ; 1c
    nop               ; 1c
    nop               ; 1c
    nop               ; 1c
    sjmp 00005$       ; 3c

  00000$:
    // don't run the DPH loop if DPH is zero
    jz   00003$       ; 3c
    // loop for DPH*256*4 cycles, DPH*512 instructions
  00001$:
      nop               ; 1c
      nop               ; 1c
      mov  a, #0xfe     ; 2c
  00002$:
        djnz acc, 00002$  ; 3c
      djnz dph, 00001$  ; 4c

  00003$:
    // don't run the DPL loop if DPL is zero
    mov  a, dpl       ; 2c
    jz   00005$       ; 3c
    // loop for DPL*4 cycles, DPL instructions
  00004$:
      djnz dpl, 00004$  ; 4c

  00005$:
#if !defined(__SDCC_MODEL_HUGE)
    ret               ; 4c
#else
    ljmp __sdcc_banked_ret
#endif
  __endasm;
}

void delay_us_overhead(uint16_t count, uint8_t caller_overh) __naked __reentrant {
  count;
  caller_overh;
  __asm;
    ; (ljmp delay_us_overhead) ; 4c

    // prolog
    ar7 = 0x07
    ar6 = 0x06
    ar1 = 0x01
    ar0 = 0x00
    // count dph:dpl
    // iters  r7:r6
    // overh  r1
    // cpucs  r0
    push ar7            ; 2c
    push ar6            ; 2c
    push ar1            ; 2c
    push ar0            ; 2c
    mov  r6, dpl        ; 2c
    mov  r7, dph        ; 2c

    // iters = count * 3
    mov  a, r6          ; 1c
    add  a, r6          ; 1c
    mov  r0, a          ; 1c
    mov  a, r7          ; 1c
    rlc  a              ; 1c
    mov  r1, a          ; 1c
    mov  a, r6          ; 1c
    add  a, r0          ; 1c
    mov  r6, a          ; 1c
    mov  a, r7          ; 1c
    addc a, r1          ; 1c
    mov  r7, a          ; 1c

    // cpucs = CPUCS
    mov  dptr, #_CPUCS  ; 3c
    movx a, @dptr       ; 2c
    mov  r0, a          ; 1c

    // overh = (48 MHz cycle tally)
    mov  r1, #(40+36)   ; 2c

    // if(cpucs & _CLKSPD1) skip
    jb   acc.4, 00000$  ; 4c => 40c

    // iters >>= 1
    clr  c              ; 1c
    mov  a, r7          ; 1c
    rrc  a              ; 1c
    xch  a, r6          ; 1c
    rrc  a              ; 1c
    xch  a, r6          ; 1c
    mov  r7, a          ; 1c

    // overh = (24 MHz cycle tally)
    mov  r1, #(40+14+36) ; 2c

    // if(cpucs & _CLKSPD0) skip
    mov  a, r0          ; 1c
    jb   acc.3, 00000$  ; 4c => 14c

    // iters >>= 1
    clr  c              ; 1c
    mov  a, r7          ; 1c
    rrc  a              ; 1c
    xch  a, r6          ; 1c
    rrc  a              ; 1c
    xch  a, r6          ; 1c
    mov  r7, a          ; 1c

    // overh = (12 MHz cycle tally)
    mov  r1, #(40+14+9+36) ; 2c => 9c

  00000$:
    // overh = (overh + caller_overh) >> 2
    mov  a, sp          ; 2c
    add  a, #-6         ; 2c
    mov  r0, a          ; 1c
    mov  a, r1          ; 1c
    add  a, @r0         ; 1c
    clr  c              ; 1c
    rrc  a              ; 1c
    clr  c              ; 1c
    rrc  a              ; 1c
    mov  r1, a          ; 1c

    // iters -= overh
    clr  c              ; 1c
    mov  a, r6          ; 1c
    subb a, r1          ; 1c
    mov  dpl, a         ; 1c
    mov  a, r7          ; 1c
    subb a, #0          ; 2c
    mov  dph, a         ; 1c

    // if(underflow) return
    jnc  00001$         ; 4c
#if !defined(__SDCC_MODEL_HUGE)
    ret
#else
    ljmp __sdcc_banked_ret
#endif

  00001$:
    // delay_4c(iters)
    pop  ar0            ; 2c
    pop  ar1            ; 2c
    pop  ar6            ; 2c
    pop  ar7            ; 2c => 36c
    ljmp _delay_4c
  __endasm;
}

void delay_us(uint16_t count) __naked {
  count;
  __asm;
    ; (mov dptr, #?)    ; 3c
    ; (ljmp delay_us)   ; 4c
    mov  a, #17         ; 2c
    push acc            ; 2c
    lcall _delay_us_overhead
    dec  sp             ; 2c
#if !defined(__SDCC_MODEL_HUGE)
    ret                 ; 4c => 17c
#else
    ljmp __sdcc_banked_ret
#endif
  __endasm;
}

void delay_ms(uint16_t count) __naked {
  count;
  __asm;
    // prolog
    ar7 = 0x07
    ar6 = 0x06
    push ar7
    push ar6
    mov  r6, dpl
    mov  r7, dph

    // overhead = (cycle tally)
    mov  a, #18
    push acc

  00000$:
    mov  a, r6          ; 1c
    orl  a, r7          ; 1c
    jz   00002$         ; 3c

    dec  r6             ; 1c
    cjne r6, #0xff, 00001$ ; 4c
    dec  r7             ; 1c

  00001$:
    mov  dpl, #(1000&0xff) ; 2c
    mov  dph, #(1000>>8)   ; 2c
    lcall _delay_us_overhead

    sjmp 00000$         ; 3c

  00002$:
    dec  sp

    // epilog
    pop  ar6
    pop  ar7
#if !defined(__SDCC_MODEL_HUGE)
    ret                 ; 4c => 17c
#else
    ljmp __sdcc_banked_ret
#endif
  __endasm;
}
