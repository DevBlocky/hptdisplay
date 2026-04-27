.extern _trap_vt
.extern init

.section .text._start
.globl _start_el2
_start_el2:
    // sctlr_el1 is uninitialized (unknown) before switching to EL1 so we
    // must give it a value. right now, all caching + MMU is disabled,
    // which is modified later in mmu.c
    mov x9, 0
    msr sctlr_el1, x9

    // skip dropping to EL1 if we're already there
    mrs x9, currentel
    lsr x9, x9, #2
    cmp x9, #2
    bne _start_el1 // already at EL1, drop into _start_el1

    // drop down to EL1

    // setup 64-bit in EL1
    mov x9, #(1 << 31) // RW (aarch64)
    orr x9, x9, #(1 << 1) // SWIO
    msr hcr_el2, x9
    // set next execution level to EL1h
    mov x9, #0x3C5
    msr spsr_el2, x9
    // set _start_el1 as the exception link register
    // (eret will jump here)
    ldr x9, =_start_el1
    msr elr_el2, x9
    // exception return (change execution level)
    isb
    eret

_start_el1:
    // install _trap_vt (for debugging exceptions)
    ldr x9, =_trap_vt
    msr vbar_el1, x9
    isb

    // disable a trap when the CPU utilizes SIMD instructions
    mov x9, #(3 << 20) // FPEN = 0b11 (no SIMD/FP traps)
    msr cpacr_el1, x9
    isb

    // put cpu id in x0 (arg0)
    // RPI3B+ has 4 cores so values are [0,3]
    mrs x0, mpidr_el1
    and x0, x0, #0xFF

    // zero BSS if on cpu0
    cbnz x0, 2f
    ldr x9, =__bss_start
    ldr x10, =__bss_end
1:
    cmp x9, x10
    bhs 2f
    str xzr, [x9], #8
    b 1b
2:

    // setup the EL1h and EL1t stack for this cpu
    // sp = .L_stackh + (cpu+1)*4096
    // sp_el0 = .L_stackt + (cpu+1)*4096
    add x9, x0, #1
    lsl x9, x9, #12 // *4096
    // sp
    ldr x10, =.L_stackh
    add x10, x9, x10
    mov sp, x10
    // sp_el0
    ldr x10, =.L_stackt
    add x10, x9, x10
    msr sp_el0, x10

    // park the core if cpu>0
    // TODO: multiple cores for task_sched
    cbz x0, 1f
    b _deadlock

    // call init
1:  bl init

_deadlock:
    wfi
    b _deadlock

// .L_stackh is the stack region for EL1h (regular operation)
// .L_stackt is the stack region for EL1t (trap/interrupt handler)
.section .data
.balign 4096
.L_stackh:
    .space 16384 // 4096*4 (4kb for each cpu)
.balign 4096
.L_stackt:
    .space 16384 // 4096*4 (4kb for each cpu)
