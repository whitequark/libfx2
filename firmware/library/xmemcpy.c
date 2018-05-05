#include <fx2lib.h>
#include <fx2regs.h>

#if defined(__SDCC_MODEL_SMALL)
#define GET_PARM(rA, rB, parm) \
    mov  rA, parm+0 \
    mov  rB, parm+1
#elif defined(__SDCC_MODEL_MEDIUM)
#define HASH #
#define GET_PARM(rA, rB, parm) \
    mov  r0, HASH parm \
    movx a, @r0 \
    mov  rA, a \
    inc  r0 \
    movx a, @r0 \
    mov  rB, a
#elif defined(__SDCC_MODEL_LARGE) || defined(__SDCC_MODEL_HUGE)
#define HASH #
#define GET_PARM(rA, rB, parm) \
    mov  dptr, HASH parm \
    movx a, @dptr \
    mov  rA, a \
    inc  dptr \
    movx a, @dptr \
    mov  rB, a
#endif

__xdata void *xmemcpy(__xdata void *dest, __xdata void *src, uint16_t length) {
  dest;
  src;
  length;
  __asm
    push _DPH0
    push _DPL0
    push _DPH1
    push _DPL1
    push _DPS

    GET_PARM(r4, r5, _xmemcpy_PARM_2)
    GET_PARM(r6, r7, _xmemcpy_PARM_3)

    // Handle edge conditions.
    // Skip the entire function if r7:r6=0.
    // If r6<>0, increment r7, since we always decrement it first
    // in the outer loop.
    // If r6=0, the inner loop underflows, which has the same effect.
    mov  a, r6
    jz   00000$
    inc  r7
  00000$:
    mov  a, r7
    jz   00002$

    // Set up autopointers.
    mov  _AUTOPTRSETUP, #0b111 ; ATPTR2INC|APTR1INC|APTREN
    mov  _AUTOPTRL1, _DPL0
    mov  _AUTOPTRH1, _DPH0
    mov  _AUTOPTRL2, r4
    mov  _AUTOPTRH2, r5

    // Set up data pointers.
    mov  _DPS, #0
    mov  dptr, #_XAUTODAT1
    mov  _DPS, #1
    mov  dptr, #_XAUTODAT2

    // Copy.
  00001$:
        movx a, @dptr     ; 2c+s
        inc  _DPS         ; 2c
        movx @dptr, a     ; 2c+s
        inc  _DPS         ; 2c
        djnz r6, 00001$   ; 4c
      djnz r7, 00001$   ; 4c

  00002$:
    pop  _DPS
    pop  _DPL1
    pop  _DPH1
    pop  _DPL0
    pop  _DPH0
  __endasm;
}
