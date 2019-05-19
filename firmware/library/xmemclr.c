#include <fx2lib.h>
#include <fx2regs.h>
#include <bits/asmargs.h>

__xdata void *xmemclr(__xdata void *dest, uint16_t length) {
  dest;
  length;
  __asm
    // Retrieve arguments.
    // _ASM_GET_PARM may use dptr, so save that first.
    mov  r2, dpl
    mov  r3, dph
    _ASM_GET_PARM2(r4, r5, _xmemclr_PARM_2)

    // Handle edge conditions.
    // Skip the entire function if r7:r6=0.
    // If r6<>0, increment r7, since we always decrement it first in the outer loop.
    // If r6=0, the inner loop underflows, which has the same effect.
    mov  a, r4
    jz   00000$
    inc  r5
  00000$:
    mov  a, r5
    jz   00002$

    // Set up autopointers.
    mov  _AUTOPTRSETUP, #0b11 ; APTR1INC|APTREN
    mov  _AUTOPTRL1, r2
    mov  _AUTOPTRH1, r3
    mov  dptr, #_XAUTODAT1
    mov  a, #0

    // Clear.
  00001$:
        movx @dptr, a          ; 2c+s
        djnz r4, 00001$        ; 4c
      djnz r5, 00001$        ; 4c

  00002$:
  __endasm;
}
