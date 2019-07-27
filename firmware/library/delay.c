#include <stdint.h>
#include <fx2regs.h>
#include <bits/asmargs.h>

// Spin for exactly max(24, DP*4) cycles (i.e. max(96, DP*16) clocks), including the call of
// this function. The implementation is a bit complicated, but the calculations in the caller
// are simpler.
//
// This was originally optimized by @whitequark, and then improved by @8051Enthusiast.
void delay_4c(uint16_t count) __naked {
  count;
  __asm
    ; (ljmp delay_4c)       ;  0c >    4c
    mov  a, dph             ;          2c
    jnz  00001$             ;          3c >  9c [A]
    mov  a, dpl             ;          2c
    add  a, #-(20/4+1)      ;          2c
    jc   00000$             ;          3c > 16c [B]
    _ASM_RET                ;          4c > 20c
  00000$:
    cjne a, #0, 00004$      ; [B] >    4c > 20c [H]
    _ASM_RET                ;          4c > 24c
  00001$:
    mov  a, dpl             ; [A] >    2c
    jz   00003$             ;          3c > 14c [C]
  00002$:
    djnz acc, 00002$        ; [C] >   4Lc > (14+4L)c [D]
                            ; [E] > 1016c > (14+1024H+4L)c [G]
  00003$:
    nop                     ; [D] >    1c
    mov  acc, #-2           ;          3c
    djnz dph, 00002$        ;          4c > (14+8+4L) [E]
    mov  a, #-((24+4)/4)    ;          2c > (24+4L) [F]
                            ; [G] >    1c
                            ;          3c
                            ;          4c > (14+8+1024H+4L)c [E]
                            ;          2c > (24+1024H+4L)c [F]
  00004$:
    djnz acc, 00004$        ; [H] >   4Lc
    _ASM_RET                ;          4c > (24+4L)c
                            ; [F] >  996c
                            ;          4c > (1024H+4L)c
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

    pop  ar0            ; 2c
    pop  ar1            ; 2c
    pop  ar6            ; 2c
    pop  ar7            ; 2c

    // if(underflow) return
    jnc  00001$         ; 4c => 36c
    _ASM_RET

  00001$:
    // else delay_4c(iters)
    ljmp _delay_4c
  __endasm;
}

void delay_us(uint16_t count) __naked {
  count;
  __asm;
    ; (mov dptr, #?)    ; 3c
    ; (lcall delay_us)  ; 4c
    mov  a, #17         ; 2c
    push acc            ; 2c
    lcall _delay_us_overhead
    dec  sp             ; 2c
    _ASM_RET            ; 4c => 17c
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
    mov  a, #17
    push acc

  00000$:
    mov  a, r6          ; 1c
    orl  a, r7          ; 1c
    jz   00002$         ; 3c

    dec  r6             ; 1c
    cjne r6, #0xff, 00001$ ; 4c
    dec  r7             ; 1c
  00001$:

    // count_us = 1000
    mov  dptr, #1000    ; 3c
    lcall _delay_us_overhead

    sjmp 00000$         ; 3c => 17c

  00002$:
    dec  sp

    // epilog
    pop  ar6
    pop  ar7
    _ASM_RET
  __endasm;
}
