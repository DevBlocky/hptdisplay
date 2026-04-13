.section .text
.globl _panic
_panic:
    // because the following code can run before (or during) MMU initialization,
    // it cannot use atomic operations because those would cause exceptions
    // (they require "normal" memory, whereas everything is "device" memory)
    //
    // so there exists a race condition where multiple CPUs could panic
    // at the exact same time and display multiple panic messages, since
    // both see _panicking==0, but either way they still fall into _hang
    //
    // TODO: inspect sctlr_el1 and, if M is set, leverage atomicity
    ldr x1, =_panicking
    ldr w2, [x1]
    cbnz w2, .L_hang
    mov w2, #1
    str w2, [x1]

    // call printf with L_panic_format as format
    // arg0 = .L_panic_format
    // arg1 = _panic arg0 (reason string)
    // arg2 = lr (link register)
    // arg2 = ESR_EL1
    // arg3 = ELR_EL1
    // arg4 = FAR_EL1
    mrs x5, far_el1 // fault address
    mrs x4, elr_el1 // instruction that caused it
    mrs x3, esr_el1 // exception class
    mov x2, lr
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
    .string "\n!!! panic !!!\nreason: %s\nlink: %hp\nexception class: %hp\nexception link: %hp\nfault address: %hp\n"
