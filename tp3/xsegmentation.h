#include <debug.h>
#include <segmem.h>

#define GDT_MASK_LIMIT_1 0x0000FFFF
#define GDT_MASK_LIMIT_2 0x000F0000
#define GDT_MASK_FLAGS 0x00F0FF00
#define GDT_MASK_BASE_2 0x000000FF
#define GDT_MASK_BASE_3 0xFF000000

#define GDT_FLAG_KERNEL_CODE                                                \
  SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_SAVL(0) | SEG_LONG(0) | SEG_SIZE(1) | \
      SEG_GRAN(1) | SEG_PRIV(0) | SEG_DESC_CODE_XR

#define GDT_FLAG_KERNEL_DATA                                                \
  SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_SAVL(0) | SEG_LONG(0) | SEG_SIZE(1) | \
      SEG_GRAN(1) | SEG_PRIV(0) | SEG_DESC_DATA_RW

#define GDT_FLAG_USER_CODE                                                  \
  SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_SAVL(0) | SEG_LONG(0) | SEG_SIZE(1) | \
      SEG_GRAN(1) | SEG_PRIV(3) | SEG_DESC_CODE_XR

#define GDT_FLAG_USER_DATA                                                  \
  SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_SAVL(0) | SEG_LONG(0) | SEG_SIZE(1) | \
      SEG_GRAN(1) | SEG_PRIV(3) | SEG_DESC_DATA_RW

void add_seg_desc(gdt_reg_t* gdtr, seg_desc_t seg_desc);
seg_desc_t build_flat_seg(uint16_t flag);
seg_desc_t build_seg_desc(uint32_t base, uint32_t limit, uint16_t flag);

void display_gdt(gdt_reg_t* gdtr);
void display_segment(seg_desc_t* seg_desc);
