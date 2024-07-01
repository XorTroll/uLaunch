// From https://github.com/switchbrew/nx-hbloader, slightly modified

.section .text.nroEntrypointTrampoline, "ax", %progbits
.align 2
.global nroEntrypointTrampoline
.type   nroEntrypointTrampoline, %function
.cfi_startproc
nroEntrypointTrampoline:

    // Reset stack pointer.
    adrp x8, __stack_top //Defined in libnx.
    ldr  x8, [x8, #:lo12:__stack_top]
    mov  sp, x8

    // Call NRO.
    blr  x2

    // Save retval
    adrp x1, g_LastTargetResult
    str  w0, [x1, #:lo12:g_LastTargetResult]

    // Reset stack pointer and load next NRO.
    adrp x8, __stack_top
    ldr  x8, [x8, #:lo12:__stack_top]
    mov  sp, x8

    b    ext_LoadTargetImpl

.cfi_endproc
