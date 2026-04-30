.section .text
.globl _switch
_switch:
    // skip saving if old is NULL
    cmn x0, #1 // checks x0 == -1
    beq 1f

    // save GPRs + lr into *old
    stp x19, x20, [x0, #0x00]
    stp x21, x22, [x0, #0x10]
    stp x23, x24, [x0, #0x20]
    stp x25, x26, [x0, #0x30]
    stp x27, x28, [x0, #0x40]
    stp x29, lr, [x0, #0x50]
    // save sp + sp_el0 into *old
    mov x9, sp
    mrs x10, sp_el0
    stp x9, x10, [x0, #0x60]
    // save FP/SIMD into *old
    stp d8, d9, [x0, #0x70]
    stp d10, d11, [x0, #0x80]
    stp d12, d13, [x0, #0x90]
    stp d14, d15, [x0, #0xA0]

1:
    // load GPRs + lr from *new
    ldp x19, x20, [x1, #0x00]
    ldp x21, x22, [x1, #0x10]
    ldp x23, x24, [x1, #0x20]
    ldp x25, x26, [x1, #0x30]
    ldp x27, x28, [x1, #0x40]
    ldp x29, lr, [x1, #0x50]
    // load sp + sp_el0 from *new
    ldp x9, x10, [x1, #0x60]
    mov sp, x9
    msr sp_el0, x10
    // load FP/SIMD from *new
    ldp d8, d9, [x1, #0x70]
    ldp d10, d11, [x1, #0x80]
    ldp d12, d13, [x1, #0x90]
    ldp d14, d15, [x1, #0xA0]

    // ret will jump to new->lr
    ret
