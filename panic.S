.section .text
.globl _panic
_panic:
    // check if the MMU (virtual address translation) is enabled
    mrs x1, sctlr_el1
    and x1, x1, #1 // M bit
    cbnz x1, .L_check_atomic // if M is set, use .L_check_atomic

.L_check_racy:
    // because the following code can run before (or during) MMU initialization,
    // it cannot use atomic operations because those would cause exceptions
    // (they require "normal" memory, whereas everything is "device" memory)
    //
    // so there exists a race condition where multiple CPUs could panic
    // at the exact same time and display multiple panic messages, since
    // both see _panicking==0, but either way they still fall into _hang
    ldr x1, =_panicking
    ldr w2, [x1]
    cbnz w2, .L_hang // already panicking
    mov w2, #1
    str w2, [x1]
    b .L_print

.L_check_atomic:
    // the MMU is enabled, which means we can atomically check if panicking
    //
    // this prevents race conditions where multiple CPUs could panic
    // at the exact same time and display multiple panic messages
    ldr x1, =_panicking
    ldaxr w2, [x1]
    cbz w2, 1f // not already panicking
    clrex
    b .L_hang
1:  mov w2, #1
    stlxr w3, w2, [x1]
    cbnz w3, .L_check_atomic // store failed (another core beat us), retry

.L_print:
    // call printf with L_panic_format as format
    // arg0 = .L_panic_format
    // arg1 = _panic arg0 (reason string)
    // arg2 = cpu id
    // arg3 = lr (link register)
    // arg4 = ESR_EL1
    // arg5 = ELR_EL1
    // arg6 = FAR_EL1
    mrs x6, far_el1   // fault address
    mrs x5, elr_el1   // instruction that caused it
    mrs x4, esr_el1   // exception class
    mov x3, lr        // link register
    mrs x2, mpidr_el1 // cpu id
    and x2, x2, #0xFF
    mov x1, x0
    ldr x0, =.L_panic_format
    bl printf

.L_hang:
    b .L_hang

.section .data
.globl _panicking
_panicking:
    .int 0

.section .rodata
.L_panic_format:
    .string "\n!!! panic !!!\nreason: %s\ncpu: %p\nlink: %hp\nexception class: %hp\nexception link: %hp\nfault address: %hp\n"
