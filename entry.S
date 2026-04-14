.extern init

.section .text._start
.globl _start_el2
_start_el2:
    // sctlr_el1 is uninitialized (unknown) before switching to EL1 so we
    // must give it a value. right now, all caching + MMU is disabled,
    // which is modified later in mmu.c
    mov x0, 0
    msr sctlr_el1, x0

    // skip dropping to EL1 if we're already there
    mrs x0, currentel
    lsr x0, x0, #2
    cmp x0, #2
    bne _start_el1 // already at EL1, drop into _start_el1

    // drop down to EL1

    // setup 64-bit in EL1
    mov x0, #(1 << 31) // RW (aarch64)
    orr x0, x0, #(1 << 1) // SWIO
    msr hcr_el2, x0
    // set next execution level to EL1h
    mov x0, #0x3C5
    msr spsr_el2, x0
    // set _start_el1 as the exception link register
    // (eret will jump here)
    ldr x0, =_start_el1
    msr elr_el2, x0
    // exception return (change execution level)
    isb
    eret

_start_el1:
    // install _trapearly_vt (for debugging exceptions)
    ldr x0, =_trapearly_vt
    msr vbar_el1, x0
    isb

    // disable a trap when the CPU utilizes SIMD instructions
    mov x0, #(3 << 20) // FPEN = 0b11 (no SIMD/FP traps)
    msr cpacr_el1, x0
    isb

    // zero BSS
    ldr x0, =__bss_start
    ldr x1, =__bss_end
1:
    cmp x0, x1
    bge 2f
    str xzr, [x0], #8
    b 1b
2:

    // setup the stack for this cpu

    // find the cpu id
    // RPI3B+ has 4 cores so values are [0,3]
    mrs x0, mpidr_el1
    and x0, x0, #0xFF
    // load stack pointer address
    // sp = stack0 + (cpu+1)*4096
    ldr x1, =stack0
    add x2, x0, #1
    lsl x2, x2, #12 // *4096
    add x2, x1, x2
    mov sp, x2

    // park the core if cpu>0
    // TODO: multiple cores for sched
    cbz x0, 1f
    b _deadlock

    // call init
1:  bl init

_deadlock:
    wfi
    b _deadlock

.section .data
// stack for all cores
// each core gets 4096 bytes
.balign 4096
stack0:
    .space 16384

.section .text
.balign 2048
// _trapearly vector table
// calls _trapearly on synchronous exceptions
_trapearly_vt:
    b _trapearly // SP0 synchronous
    .balign 0x80
    b _trapearly // SP0 irq/virq
    .balign 0x80
    b _trapearly // SP0 fiq/vfiq
    .balign 0x80
    b _trapearly // SP0 serror/vserror
    .balign 0x80
    b _trapearly // SPx synchronous
    .balign 0x80
    b _trapearly // SPx irq/virq
    .balign 0x80
    b _trapearly // SPx fiq/vfiq
    .balign 0x80
    b _trapearly // SPx serror/vserror
    .balign 0x80
    b _trapearly // lower EL AArch64 synchronous
    .balign 0x80
    b _trapearly // lower EL AArch64 irq/virq
    .balign 0x80
    b _trapearly // lower EL AArch64 fiq/vfiq
    .balign 0x80
    b _trapearly // lower EL AArch64 serror/vserror
    .balign 0x80
    b _trapearly // lower EL AArch32 synchronous
    .balign 0x80
    b _trapearly // lower EL AArch32 irq/virq
    .balign 0x80
    b _trapearly // lower EL AArch32 fiq/vfiq
    .balign 0x80
    b _trapearly // lower EL AArch32 serror/vserror

_trapearly:
    ldr x0, =.L_trapearly_panic
    bl _panic
    // no return from _panic

.section .rodata
.L_trapearly_panic:
    .string "_trapearly hit unexpectedly"
