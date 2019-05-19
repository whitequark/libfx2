#ifndef _ASMARGS_H
#define _ASMARGS_H

#define _ASM_HASH #

#define _ASM_REG(x) _ ## x

#if defined(__SDCC_MODEL_SMALL)
#define _ASM_GET_PARM1(rA, parm) \
    mov  rA, parm+0
#elif defined(__SDCC_MODEL_MEDIUM)
#define _ASM_GET_PARM1(rA, parm) \
    mov  r0, _ASM_HASH parm \
    movx a, @r0 \
    mov  rA, a
#elif defined(__SDCC_MODEL_LARGE) || defined(__SDCC_MODEL_HUGE)
#define _ASM_GET_PARM1(rA, parm) \
    mov  dptr, _ASM_HASH parm \
    movx a, @dptr \
    mov  rA, a
#endif

#if defined(__SDCC_MODEL_SMALL)
#define _ASM_GET_PARM2(rA, rB, parm) \
    mov  rA, parm+0 \
    mov  rB, parm+1
#elif defined(__SDCC_MODEL_MEDIUM)
#define _ASM_GET_PARM2(rA, rB, parm) \
    mov  r0, _ASM_HASH parm \
    movx a, @r0 \
    mov  rA, a \
    inc  r0 \
    movx a, @r0 \
    mov  rB, a
#elif defined(__SDCC_MODEL_LARGE) || defined(__SDCC_MODEL_HUGE)
#define _ASM_GET_PARM2(rA, rB, parm) \
    mov  dptr, _ASM_HASH parm \
    movx a, @dptr \
    mov  rA, a \
    inc  dptr \
    movx a, @dptr \
    mov  rB, a
#endif

#if !defined(__SDCC_MODEL_HUGE)
#define _ASM_RET \
    ret
#else
#define _ASM_RET \
    ljmp __sdcc_banked_ret
#endif

#endif
