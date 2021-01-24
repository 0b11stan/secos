/* GPLv2 (c) Airbus */
#include <debug.h>
#include <info.h>

extern info_t* info;

void user1() {
  debug("=== ENTER USER 1 ===\n");
  // TODO: increment shared counter
  // TODO: infinit loop
}

void user2() {
  debug("=== ENTER USER 2 ===\n");
  // TODO: syscall to display shared counter
  // TODO: infinit loop
}

void userland() {
  debug("== ENTER USERLAND ==\n");

  // TODO: launch task 1
  // TODO: launch task 2

  while (1) {
  }
}

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

void enter_userland(uint32_t* userland_addr) {
  set_ds(d3_sel);
  set_es(d3_sel);
  set_fs(d3_sel);
  set_gs(d3_sel);

  TSS.s0.esp = get_ebp();
  TSS.s0.ss = d0_sel;
  tss_dsc(&GDT[ts_idx], (offset_t)&TSS);
  set_tr(ts_sel);

  asm volatile(
      "push %0    \n"  // ss
      "push %%ebp \n"  // esp
      "pushf      \n"  // eflags
      "push %1    \n"  // cs
      "push %2    \n"  // eip
      "iret" ::"i"(d3_sel),
      "i"(c3_sel), "r"(userland_addr));
}

void tp() {
  init_gdt();
  enter_userland((uint32_t*)&userland);
  // TODO: configure segmentation
  // TODO: configure pagination (identity mapping)
  // TODO: configure interruption
}
