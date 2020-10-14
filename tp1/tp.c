/* GPLv2 (c) Airbus */
#include <debug.h>
#include <info.h>
#include <segmem.h>
#include <string.h>

extern info_t* info;

void display_segment(seg_desc_t* seg_desc) {
  debug("\n### SEGMENT DESCRIPTOR @%p ###\n", seg_desc);

  uint32_t limit = seg_desc->limit_2 << 16;
  limit = limit | seg_desc->limit_1;

  uint32_t base = seg_desc->base_3 << 24;
  base = base | (seg_desc->base_2 << 16);
  base = base | (seg_desc->base_1);

  debug("base_1: %p\n", seg_desc->base_1);
  debug("base         : %p\n", base);
  debug("limit        : %p\n", limit);
  debug("segment type : %u\n", seg_desc->type);
  debug("reserved for : %s\n", seg_desc->s ? "data and code" : "system");
  debug("privilege    : %u\n", seg_desc->dpl);
  debug("activation   : %s\n", seg_desc->p ? "enabled" : "disabled");
  debug("availability : %s\n", seg_desc->avl ? "available" : "busy");
  debug("mode         : %s\n", seg_desc->l ? "64b" : "32b");
}

void tp() {
  gdt_reg_t gdtr;
  get_gdtr(gdtr);
  long unsigned int last_address = gdtr.addr + gdtr.limit + 1;

  debug("\n### GDTR ###\n");
  debug("start : %p\n", gdtr.addr);
  debug("end   : %p\n", last_address);

  seg_desc_t* seg_desc = gdtr.desc;

  while ((unsigned int)seg_desc <= last_address) {
    display_segment(seg_desc);
    seg_desc++;
  }
  debug("\n");
}
