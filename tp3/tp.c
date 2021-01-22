/* GPLv2 (c) Airbus */
#include <debug.h>
#include <info.h>

#include "xsegmentation.h"

gdt_reg_t gdtr;

extern info_t *info;

/* TP 3.2 */
void userland() { asm volatile("mov %eax, %cr0"); }

void tp() {
  /* TP 3.1 */
  add_seg_desc(&gdtr, build_seg_desc(0x0, 0x0, 0x0));
  add_seg_desc(&gdtr, build_flat_seg(GDT_FLAG_KERNEL_CODE));
  add_seg_desc(&gdtr, build_flat_seg(GDT_FLAG_KERNEL_DATA));
  add_seg_desc(&gdtr, build_flat_seg(GDT_FLAG_USER_CODE));
  add_seg_desc(&gdtr, build_flat_seg(GDT_FLAG_USER_DATA));

  //display_gdt(&gdtr);

  /* TP 3.3 */
  //int code_seg_sel = gdt_usr_seg_sel(3);
  //debug("user   : %p\n", code_seg_sel);
  //debug("kernel : %p\n", gdt_krn_seg_sel(3));

   /* fptr32_t fptr = {.segment = c3_sel, .offset = (uint32_t)userland}; */

  set_cs(gdt_krn_seg_sel(1));
  set_ds(gdt_krn_seg_sel(2));
  set_ss(gdt_krn_seg_sel(2));
  set_es(gdt_krn_seg_sel(2));
  set_fs(gdt_krn_seg_sel(2));
  set_gs(gdt_krn_seg_sel(2));

//  set_cs(gdt_usr_seg_sel(3));
//  set_ds(gdt_usr_seg_sel(4));
//  set_ss(gdt_usr_seg_sel(4));
//  set_es(gdt_usr_seg_sel(4));
//  set_fs(gdt_usr_seg_sel(4));
  set_gs(gdt_usr_seg_sel(4));
}
