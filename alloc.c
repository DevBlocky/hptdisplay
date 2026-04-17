#include "hptdisplay.h"
#include "rpi3b.h"

struct run {
  struct run *next;
};
static struct run *freelist;

void *alloc_page(void) {
  void *page = (void *)freelist;
  if (page == NULL)
    panic("alloc_page: out of memory");
  freelist = freelist->next;
  return page;
}
void alloc_freepage(void *page) {
  struct run *free = (struct run *)page;
  free->next = freelist;
  freelist = free;
}

extern char __program_start;
extern char __program_end;
static inline int overlaps(usize start, usize end) {
  usize pstart = (usize)&__program_start;
  usize pend = (usize)&__program_end;
  return start < pend && pstart < end;
}
void alloc_init(void) {
  // add all page regions that don't overlap with
  // the program memory to the freelist
  freelist = NULL;
  int count = 0;
  for (usize addr = SDRAM_START; addr < SDRAM_END; addr += PAGESZ) {
    if (!overlaps(addr, addr + PAGESZ)) {
      alloc_freepage((void *)addr);
      count++;
    }
  }
  // print how many pages of memory we have available
  printf("allocator: total pages = %d\n", count);
}
