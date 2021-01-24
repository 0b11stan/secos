/* GPLv2 (c) Airbus */
#include <asm.h>
#include <debug.h>
#include <info.h>
#include <intr.h>

#include "xsegmentation.h"

extern info_t* info;

seg_desc_t GDT[6];
tss_t TSS;

void init_gdt() {
  gdt_reg_t gdtr;

  GDT[0].raw = 0ULL;

  c0_dsc(&GDT[c0_idx]);
  d0_dsc(&GDT[d0_idx]);
  c3_dsc(&GDT[c3_idx]);
  d3_dsc(&GDT[d3_idx]);

  gdtr.desc = GDT;
  gdtr.limit = sizeof(GDT) - 1;
  set_gdtr(gdtr);

  set_cs(c0_sel);

  set_ss(d0_sel);
  set_ds(d0_sel);
  set_es(d0_sel);
  set_fs(d0_sel);
  set_gs(d0_sel);
}

void user1() { debug("ENTER USER 1\n"); }

void user2() { debug("ENTER USER 2\n"); }

void interrupt_clock() {
  static int cmpt = 0;
  if (cmpt % 2 == 0)
    user1();
  else
    user2();
  cmpt++;
}

void tp() {
  init_gdt();
  intr_init();

  register_gate(32, &interrupt_clock);
  force_interrupts_on();

  while (1) {
  }
}
