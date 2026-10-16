// #include "defines.h"

.section .text.init
.global _start
_start:

// enable caching, except region 0xd
        li t0, 0x59555555
        csrw 0x7c0, t0

        .option push
        .option norelax
        la gp, __global_pointer$
        .option pop
        la sp, STACK

        la t0, __bss_start
        la t1, __bss_end
1:
        bgeu t0, t1, 2f
        sw zero, 0(t0)
        addi t0, t0, 4
        j 1b
2:
        la t0, __tflm_arena_start
        la t1, __tflm_arena_end
3:
        bgeu t0, t1, 4f
        sw zero, 0(t0)
        addi t0, t0, 4
        j 3b
4:
        call __libc_init_array
        call main
        bnez a0, _fail


.global _finish
_finish:
        la t0, tohost
        li t1, 0xff
        sb t1, 0(t0) // DemoTB test termination
        li t1, 1
        sw t1, 0(t0) // Whisper test termination
        beq x0, x0, _finish
        .rept 10
        nop
        .endr

_fail:
        la t0, tohost
        li t1, 1
        sw t1, 0(t0)
        beq x0, x0, _fail

.section .data.io
.global tohost
tohost: .word 0
