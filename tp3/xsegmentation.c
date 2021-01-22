#include <segmem.h>
#include "xsegmentation.h"

void add_seg_desc(gdt_reg_t* gdtr, seg_desc_t seg_desc) {
  int nb_segments = (gdtr->limit + 1) / sizeof(seg_desc_t);
  gdtr->desc[nb_segments] = seg_desc;
  gdtr->limit = ((nb_segments+1) * sizeof(seg_desc_t)) - 1;
}

seg_desc_t build_flat_seg(uint16_t flag) {
  return build_seg_desc(0x0, 0xffffffff, flag);
}

seg_desc_t build_seg_desc(uint32_t base, uint32_t limit, uint16_t flag) {
  uint64_t descriptor;

  // Create the high 32 bit segment
  descriptor = limit & GDT_MASK_LIMIT_2;
  descriptor |= (flag << 8) & GDT_MASK_FLAGS;
  descriptor |= (base >> 16) & GDT_MASK_BASE_2;
  descriptor |= base & GDT_MASK_BASE_3;

  // Shift by 32 to allow for low part of segment
  descriptor <<= 32;

  // Create the low 32 bit segment
  descriptor |= base << 16;
  descriptor |= limit & GDT_MASK_LIMIT_1;  // set limit bits 15:0

  seg_desc_t result;
  result.raw = descriptor;
  return result;
}

void display_gdt(gdt_reg_t* gdtr) {
  long unsigned int last_address = gdtr->addr + gdtr->limit + 1;

  debug("\n### GDTR ###\n");
  debug("start : %p\n", gdtr->addr);
  debug("end   : %p\n", last_address);

  seg_desc_t* seg_desc = gdtr->desc;

  while ((unsigned int)seg_desc < last_address) {
    display_segment(seg_desc);
    seg_desc++;
  }
}

void display_segment(seg_desc_t* seg_desc) {
  debug("\n### SEGMENT DESCRIPTOR @%p ###\n", seg_desc);

  uint32_t limit = seg_desc->limit_2 << 16;
  limit = limit | seg_desc->limit_1;

  uint32_t base = seg_desc->base_3 << 24;
  base = base | (seg_desc->base_2 << 16);
  base = base | (seg_desc->base_1);

  debug("base         : %p\n", base);
  debug("limit        : %p\n", limit);
  debug("seg type     : %p\n", seg_desc->type);
  debug("desc type    : %p\n", seg_desc->s);
  debug("privilege    : %p\n", seg_desc->dpl);
  debug("activation   : %p\n", seg_desc->p);
  debug("availability : %p\n", seg_desc->avl);
  debug("mode         : %p\n", seg_desc->l);
}
