/* GPLv2 (c) Airbus */
#include <debug.h>
#include <info.h>
#include <segmem.h>
#include <string.h>

extern info_t* info;

gdt_reg_t new_gdt; // obligé car pas encore de malloc doit survivre à la stack

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
  debug("seg type     : %u\n", seg_desc->type);
  debug("desc type    : %s\n", seg_desc->s ? "data and code" : "system");
  debug("privilege    : %u\n", seg_desc->dpl);
  debug("activation   : %s\n", seg_desc->p ? "enabled" : "disabled");
  debug("availability : %s\n", seg_desc->avl ? "available" : "busy");
  debug("mode         : %s\n", seg_desc->l ? "64b" : "32b");
}

void init_seg_desc(seg_desc_t* seg_desc,
                  uint32_t base,
                  uint32_t limit,
                  uint8_t seg_type,
                  uint8_t desc_type,
                  uint8_t privilege,
                  uint8_t enabled,
                  uint8_t mode) {
  // lire https://wiki.osdev.org/GDT_Tutorial
  // utiliser https://wiki.osdev.org/GDT_Tutorial#Some_stuff_to_make_your_life_easy
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

  debug("SS : %p\n", get_ss());
  debug("DS : %p\n", get_ds());
  debug("ES : %p\n", get_es());
  debug("FS : %p\n", get_fs());
  debug("GS : %p\n", get_gs());


  seg_desc_t code_seg;

  init_seg_desc(); // TODO : for code
  init_seg_desc(); // TODO : for data

  // TODO : compute gdt from given segments
  
  set_gdtr(&new_gdt);

  debug("\n");
}
