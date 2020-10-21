/* GPLv2 (c) Airbus */
#include <debug.h>
#include <info.h>
#include <segmem.h>
#include <string.h>

#define GDT_MASK_LIMIT_1 0x0000FFFF
#define GDT_MASK_LIMIT_2 0x000F0000
#define GDT_MASK_FLAGS   0x00F0FF00
#define GDT_MASK_BASE_2  0x000000FF
#define GDT_MASK_BASE_3  0xFF000000

#define GDT_FLAG_CODE SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_SAVL(0) | \
                      SEG_LONG(0)     | SEG_SIZE(1) | SEG_GRAN(1) | \
                      SEG_PRIV(0)     | SEG_DESC_CODE_XR
 
#define GDT_FLAG_DATA SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_SAVL(0) | \
                      SEG_LONG(0)     | SEG_SIZE(1) | SEG_GRAN(1) | \
                      SEG_PRIV(0)     | SEG_DESC_DATA_RW 

extern info_t* info;

int nb_segments = 0;
gdt_reg_t gdtr; // obligé car pas encore de malloc
                // et le tableau doit survivre à la stack

//const seg_sel_t cs = { .rpl = SEG_SEL_KRN, .ti = SEG_SEL_GDT, .index = 0x1 };
const uint16_t cs = 8; // I don't understand why the line above do not work
const seg_sel_t ss = { .rpl = SEG_SEL_KRN, .ti = SEG_SEL_GDT, .index = 0x2 };
const seg_sel_t ds = { .rpl = SEG_SEL_KRN, .ti = SEG_SEL_GDT, .index = 0x2 };


void display_gdt(gdt_reg_t* gdtr);
void display_segment(seg_desc_t* seg_desc);
void add_seg_desc(gdt_reg_t* gdtr, seg_desc_t seg_desc);
seg_desc_t build_seg_desc(uint32_t base, uint32_t limit, uint16_t flags);

void tp() {
  /* Question 1 
   * ----------
   * gdt_reg_t gdtr;
   * get_gdtr(gdtr);
   * display_gdt(&gdtr);
  **/

  add_seg_desc(&gdtr, build_seg_desc(0x0, 0x0, 0x0));
  add_seg_desc(&gdtr, build_seg_desc(0x0, 0xffffffff, GDT_FLAG_CODE));
  add_seg_desc(&gdtr, build_seg_desc(0x0, 0xffffffff, GDT_FLAG_DATA));

  display_gdt(&gdtr);

  set_cs(cs);
  set_ss(ss.raw);
  set_ds(ds.raw);

  debug("SS : %p\n", get_ss());
  debug("DS : %p\n", get_ds());

  /*
  debug("SS : %p\n", get_ss());
  debug("DS : %p\n", get_ds());
  debug("ES : %p\n", get_es());
  debug("FS : %p\n", get_fs());
  debug("GS : %p\n", get_gs());
  */

  debug("\n");
}

void add_seg_desc(gdt_reg_t* gdtr, seg_desc_t seg_desc) {
  gdtr->desc[nb_segments] = seg_desc;
  gdtr->limit = nb_segments * sizeof(seg_desc_t);
  nb_segments++;
}

void display_gdt(gdt_reg_t* gdtr) {
  long unsigned int last_address = gdtr->addr + gdtr->limit + 1;

  debug("\n### GDTR ###\n");
  debug("start : %p\n", gdtr->addr);
  debug("end   : %p\n", last_address);

  seg_desc_t* seg_desc = gdtr->desc;

  while ((unsigned int)seg_desc <= last_address) {
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
  debug("seg type     : %x\n", seg_desc->type);
  debug("desc type    : %s\n", seg_desc->s ? "data or code" : "system");
  debug("privilege    : %u\n", seg_desc->dpl);
  debug("activation   : %s\n", seg_desc->p ? "enabled" : "disabled");
  debug("availability : %s\n", seg_desc->avl ? "available" : "busy");
  debug("mode         : %s\n", seg_desc->l ? "64b" : "32b");
}

seg_desc_t build_seg_desc(uint32_t base, uint32_t limit, uint16_t flag) {
  uint64_t descriptor;

  // Create the high 32 bit segment
  descriptor  =  limit       & GDT_MASK_LIMIT_2;
  descriptor |= (flag <<  8) & GDT_MASK_FLAGS;
  descriptor |= (base >> 16) & GDT_MASK_BASE_2;
  descriptor |=  base        & GDT_MASK_BASE_3;

  // Shift by 32 to allow for low part of segment
  descriptor <<= 32;

  // Create the low 32 bit segment
  descriptor |= base  << 16;
  descriptor |= limit & GDT_MASK_LIMIT_1; // set limit bits 15:0

  seg_desc_t result;
  result.raw = descriptor;
  return result;
}
