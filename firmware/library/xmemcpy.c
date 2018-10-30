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
    // Retrieve arguments.
    // GET_PARM may use dptr, so save that first.
    mov  r2, dpl
    mov  r3, dph
    GET_PARM(r4, r5, _xmemcpy_PARM_2)
    GET_PARM(r6, r7, _xmemcpy_PARM_3)

    // Handle edge conditions.
    // Skip the entire function if r7:r6=0.
    // If r6<>0, increment r7, since we always decrement it first in the outer loop.
    // If r6=0, the inner loop underflows, which has the same effect.
    mov  a, r6
    jz   00000$
    inc  r7
  00000$:
    mov  a, r7
    jz   00002$

    // Set up autopointers.
    mov  _AUTOPTRSETUP, #0b111 ; ATPTR2INC|APTR1INC|APTREN
    mov  _AUTOPTRL1, r2
    mov  _AUTOPTRH1, r3
    mov  _AUTOPTRL2, r4
    mov  _AUTOPTRH2, r5

    // Copy.
  00001$:
        // We could save 2c per iteration by using `inc _DPS` instead of explicitly loading dptr,
        // but unfortunately, ISRs save dpl/dph which maps to _DPL0/_DPH0, but at the same time
        // use `mov dptr` which maps to the data pointer indexed by _DPS. We cannot fix this
        // without either (a) disabling interrupts within xmemcpy, increasing latency, or
        // (b) requiring all interrupts to carefully insert custom prologue/epilogue code,
        // both of which are undesirable. So, we just eat the increased cost. (It's quite a bit
        // faster than the naive memcpy, anyway.)
        mov  dptr, #_XAUTODAT2 ; 3c
        movx a, @dptr          ; 2c+s
        mov  dptr, #_XAUTODAT1 ; 3c
        movx @dptr, a          ; 2c+s
        djnz r6, 00001$        ; 4c
      djnz r7, 00001$        ; 4c

  00002$:
  __endasm;
}
