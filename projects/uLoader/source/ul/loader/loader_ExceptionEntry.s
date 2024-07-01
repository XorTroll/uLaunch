// From https://github.com/switchbrew/nx-hbloader, slightly modified

.section .text.__libnx_exception_entry, "ax", %progbits
.global __libnx_exception_entry
.type __libnx_exception_entry, %function
.align 2
.cfi_startproc
__libnx_exception_entry:
    // Divert execution to the NRO entrypoint (if a NRO is actually loaded)
    adrp x7, g_TargetMapAddress
    ldr x7, [x7, #:lo12:g_TargetMapAddress]
    cbz x7, __libnx_exception_entry_fail
    br x7

__libnx_exception_entry_fail:
    mov w0, #0xF801 // ResultNotHandled
    svc 0x28 // svcReturnFromException
.cfi_endproc
