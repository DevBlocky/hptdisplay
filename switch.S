.section .text
.globl _switch
_switch:
    // skip saving if old is NULL
    cmn x0, #1 // checks x0 == -1
    beq 1f

    // save GPRs into *old
    stp x19, x20, [x0, #0x00]
    stp x21, x22, [x0, #0x10]
    stp x23, x24, [x0, #0x20]
    stp x25, x26, [x0, #0x30]
    stp x27, x28, [x0, #0x40]
    str x29, [x0, #0x50]
    // save lr+sp into *old
    mov x9, sp
    stp lr, x9, [x0, #0x58]
    // save FP/SIMD into *old
    stp d8, d9, [x0, #0x68]
    stp d10, d11, [x0, #0x78]
    stp d12, d13, [x0, #0x88]
    stp d14, d15, [x0, #0x98]

1:
    // load GPRs from *new
    ldp x19, x20, [x1, #0x00]
    ldp x21, x22, [x1, #0x10]
    ldp x23, x24, [x1, #0x20]
    ldp x25, x26, [x1, #0x30]
    ldp x27, x28, [x1, #0x40]
    ldr x29, [x1, #0x50]
    // load lr+sp from *new
    ldp lr, x9, [x1, #0x58]
    mov sp, x9
    // load FP/SIMD from *new
    ldp d8, d9, [x1, #0x68]
    ldp d10, d11, [x1, #0x78]
    ldp d12, d13, [x1, #0x88]
    ldp d14, d15, [x1, #0x98]

    // ret will jump to new->lr
    ret
