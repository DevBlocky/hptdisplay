#include "hptdisplay.h"
#include "rpi3b.h"

#define TT_AF (1ULL << 10)         // access Flag
#define TT_SH_INNER (3ULL << 8)    // inner shareable
#define TT_AP_RW_EL1 (0ULL << 6)   // R/W EL1 only
#define TT_ATTR_NORMAL (0ULL << 2) // MAIR Attr0 = normal memory
#define TT_ATTR_DEVICE (1ULL << 2) // MAIR Attr1 = device memory
#define TT_BLOCK_ENTRY (1ULL << 0) // block descriptor

#define L2_BLOCK_SIZE (2ULL * 1024 * 1024) // 2MB per L2 block entry
#define TT_ENTRY_COUNT 512

// level 2 translation table (covers first 1GB in 2MB blocks)
// since TCR_T0 is 30-bits, our root translation table is actually L2
static u64 l2_table[TT_ENTRY_COUNT] __attribute__((aligned(4096)));

void mmu_inittables(void) {
  for (usize i = 0; i < TT_ENTRY_COUNT; i++)
    l2_table[i] = 0;

  // create normal memory block entries for SDRAM
  for (usize addr = SDRAM_START; addr < SDRAM_END; addr += L2_BLOCK_SIZE)
    l2_table[addr / L2_BLOCK_SIZE] = addr | TT_BLOCK_ENTRY | TT_AF |
                                     TT_SH_INNER | TT_AP_RW_EL1 |
                                     TT_ATTR_NORMAL;

  // create device memory block entries for MMIO
  for (usize addr = MMIO_START; addr < MMIO_END; addr += L2_BLOCK_SIZE)
    l2_table[addr / L2_BLOCK_SIZE] =
        addr | TT_BLOCK_ENTRY | TT_AF | TT_AP_RW_EL1 | TT_ATTR_DEVICE;
}

#define MAIR_NORMAL_WB                                                         \
  (0xFF << 0) // Attr0: 0xFF = normal, inner/outer write-back cacheable
#define MAIR_DEVICE_nGnRnE                                                     \
  (0x00 << 8) // Attr1: 0x00 = device-nGnRnE (strongly ordered)

#define TCR_T0_30BIT ((64ULL - 30) << 0) // 30-bit (1GB) VA space
#define TCR_IRGN0_WB (1ULL << 8)         // inner write-back, read-allocate
#define TCR_ORGN0_WB (1ULL << 10)        // outer write-back, read-allocate
#define TCR_SH0_INNER (3ULL << 12)       // inner shareable
#define TCR_TG0_4KB (0ULL << 14)         // 4KB granule (L3 block size)
#define TCR_EPD1 (1ULL << 23)      // disable TTBR1 (user translation table)
#define TCR_IPS_36BIT (1ULL << 32) // 36-bit (64GB) PA space

#define SCTLR_M (1ULL << 0)  // M = enable MMU
#define SCTLR_C (1ULL << 2)  // C = enable data cache
#define SCTLR_I (1ULL << 12) // I = enable instruction cache

void mmu_initcpu(void) {
  // set MAIR_EL1 (memory attribute register)
  asm volatile("msr mair_el1, %0" ::"r"(MAIR_NORMAL_WB | MAIR_DEVICE_nGnRnE));
  // set TCR_EL1 (translation control)
  asm volatile("msr tcr_el1, %0" ::"r"(TCR_T0_30BIT | TCR_IRGN0_WB |
                                       TCR_ORGN0_WB | TCR_SH0_INNER |
                                       TCR_TG0_4KB | TCR_EPD1 | TCR_IPS_36BIT));
  // set TTBR0_EL1 to point to our L2 table
  asm volatile("msr ttbr0_el1, %0" ::"r"((u64)&l2_table));
  asm volatile("isb"); // instruction synchronization barrier

  // invalidate all TLB entries
  asm volatile("tlbi vmalle1"); // invalidate all TLB entries at EL1
  asm volatile("dsb sy"); // wait for the invalidation to actually complete
  asm volatile("isb");    // instruction synchronization barrier

  // set SCTLR_EL1 to enable MMU and caching
  u64 sctlr_el1;
  asm volatile("mrs %0, sctlr_el1" : "=r"(sctlr_el1));
  sctlr_el1 |= SCTLR_M | SCTLR_C | SCTLR_I;
  asm volatile("msr sctlr_el1, %0" ::"r"(sctlr_el1));
  asm volatile("isb"); // instruction synchronization barrier
}
