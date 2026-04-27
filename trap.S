.extern intr_handle

.section .text
.globl _trap_vt
.balign 2048
// _trap vector table
// each interrupt/exception calls .L_trap
_trap_vt:
    b _trap_sync // SP0 synchronous
    .balign 0x80
    b _trap_irq // SP0 irq/virq
    .balign 0x80
    b _trap_err // SP0 fiq/vfiq
    .balign 0x80
    b _trap_err // SP0 serror/vserror
    .balign 0x80
    b _trap_sync // SPx synchronous
    .balign 0x80
    b _trap_irq // SPx irq/virq
    .balign 0x80
    b _trap_err // SPx fiq/vfiq
    .balign 0x80
    b _trap_err // SPx serror/vserror
    // omit lower EL traps since they will never invoke

_trap_irq:
    msr spsel, #0 // use EL1t (interrupt handler stack)

    // push all GPRs to stack
    sub sp, sp, #0xF8 // 31 GSRs
    stp x0, x1, [sp, #0x00]
    stp x2, x3, [sp, #0x10]
    stp x4, x5, [sp, #0x20]
    stp x6, x7, [sp, #0x30]
    stp x8, x9, [sp, #0x40]
    stp x10, x11, [sp, #0x50]
    stp x12, x13, [sp, #0x60]
    stp x14, x15, [sp, #0x70]
    stp x16, x17, [sp, #0x80]
    stp x18, x19, [sp, #0x90]
    stp x20, x21, [sp, #0xA0]
    stp x22, x23, [sp, #0xB0]
    stp x24, x25, [sp, #0xC0]
    stp x26, x27, [sp, #0xD0]
    stp x28, x29, [sp, #0xE0]
    str x30, [sp, #0xF0]
    // push ELR and SPSR (where to return to, and what state)
    sub sp, sp, #0x10 // elr_el1 + spsr_el1
    mrs x10, elr_el1
    mrs x11, spsr_el1
    stp x10, x11, [sp]
    // push SIMD/FP registers to stack
    sub sp, sp, #0x200 // 32 Q regs (0x200)
    stp q0, q1, [sp, #0x000]
    stp q2, q3, [sp, #0x020]
    stp q4, q5, [sp, #0x040]
    stp q6, q7, [sp, #0x060]
    stp q8, q9, [sp, #0x080]
    stp q10, q11, [sp, #0x0A0]
    stp q12, q13, [sp, #0x0C0]
    stp q14, q15, [sp, #0x0E0]
    stp q16, q17, [sp, #0x100]
    stp q18, q19, [sp, #0x120]
    stp q20, q21, [sp, #0x140]
    stp q22, q23, [sp, #0x160]
    stp q24, q25, [sp, #0x180]
    stp q26, q27, [sp, #0x1A0]
    stp q28, q29, [sp, #0x1C0]
    stp q30, q31, [sp, #0x1E0]
    sub sp, sp, #0x10 // fpcr + fpsr
    mrs x10, fpcr
    mrs x11, fpsr
    stp x10, x11, [sp]

    // call irq handler
    bl intr_handle

    // pop SIMD/FP registers from stack
    ldp x10, x11, [sp]
    msr fpsr, x11
    msr fpcr, x10
    add sp, sp, #0x10
    ldp q30, q31, [sp, #0x1E0]
    ldp q28, q29, [sp, #0x1C0]
    ldp q26, q27, [sp, #0x1A0]
    ldp q24, q25, [sp, #0x180]
    ldp q22, q23, [sp, #0x160]
    ldp q20, q21, [sp, #0x140]
    ldp q18, q19, [sp, #0x120]
    ldp q16, q17, [sp, #0x100]
    ldp q14, q15, [sp, #0x0E0]
    ldp q12, q13, [sp, #0x0C0]
    ldp q10, q11, [sp, #0x0A0]
    ldp q8, q9, [sp, #0x080]
    ldp q6, q7, [sp, #0x060]
    ldp q4, q5, [sp, #0x040]
    ldp q2, q3, [sp, #0x020]
    ldp q0, q1, [sp, #0x000]
    add sp, sp, #0x200
    // pop ELR and SPSR from stack
    ldp x10, x11, [sp]
    msr spsr_el1, x11
    msr elr_el1, x10
    add sp, sp, #0x10
    // pop all GPRs from stack
    ldr x30, [sp, #0xF0]
    ldp x28, x29, [sp, #0xE0]
    ldp x26, x27, [sp, #0xD0]
    ldp x24, x25, [sp, #0xC0]
    ldp x22, x23, [sp, #0xB0]
    ldp x20, x21, [sp, #0xA0]
    ldp x18, x19, [sp, #0x90]
    ldp x16, x17, [sp, #0x80]
    ldp x14, x15, [sp, #0x70]
    ldp x12, x13, [sp, #0x60]
    ldp x10, x11, [sp, #0x50]
    ldp x8, x9, [sp, #0x40]
    ldp x6, x7, [sp, #0x30]
    ldp x4, x5, [sp, #0x20]
    ldp x2, x3, [sp, #0x10]
    ldp x0, x1, [sp, #0x00]
    add sp, sp, #0xF8

    msr spsel, #1 // switch back to EL1h
    eret // return to previous execution

_trap_sync:
    ldr x0, =.L_trap_sync_panic
    bl _panic
    // no return from _panic
_trap_err:
    ldr x0, =.L_trap_err_panic
    bl _panic
    // no return from _panic

.section .rodata
.L_trap_sync_panic:
    .string "_trap_sync hit unexpectedly"
.L_trap_err_panic:
    .string "_trap_err hit unexpectedly"
