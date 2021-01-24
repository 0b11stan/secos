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
  tss_dsc(&GDT[ts_idx], (offset_t)&TSS);

  gdtr.desc = GDT;
  gdtr.limit = sizeof(GDT) - 1;
  set_gdtr(gdtr);

  set_cs(c0_sel);

  set_ss(d0_sel);
  set_ds(d0_sel);
  set_es(d0_sel);
  set_fs(d0_sel);
  set_gs(d0_sel);
  set_tr(ts_sel);
}

void user1() {
  debug("START TASK 1\n");
  while (1) {
    //force_interrupts_on();
    debug("running task 1 ...\n");
  }
  // asm volatile("mov %eax, %cr0"); // fail if we are in ring3
}

void user2() {
  debug("START TASK 2\n");
  while (1) {
    force_interrupts_on();
    debug("running task 2 ...\n");
  }
  // asm volatile("mov %eax, %cr0"); // fail if we are in ring3
}

void init_userland_1() {
  debug("INIT TASK 1\n");

  set_ds(d3_sel);
  set_es(d3_sel);
  set_fs(d3_sel);
  set_gs(d3_sel);

  TSS.s0.esp = get_ebp();
  TSS.s0.ss = d0_sel;

  uint32_t ustack = 0x600000;

  force_interrupts_on();
  asm volatile(
      "push %0 \n"  // ss
      "push %1 \n"  // esp
      "pushf   \n"  // eflags
      "push %2 \n"  // cs
      "push %3 \n"  // eip
      "iret" ::"i"(d3_sel),
      "m"(ustack), "i"(c3_sel), "r"(&user1));
}

void enter_userland_1(uint32_t esp, uint32_t eip) {
  debug("ENTER TASK 1\n");
  set_ds(d3_sel);
  set_es(d3_sel);
  set_fs(d3_sel);
  set_gs(d3_sel);

  TSS.s0.esp = get_ebp();
  TSS.s0.ss = d0_sel;

  asm volatile(
      "push %0 \n"  // ss
      "push %1 \n"  // esp
      "pushf   \n"  // eflags
      "push %2 \n"  // cs
      "push %3 \n"  // eip
      "iret" ::"i"(d3_sel),
      "m"(esp), "i"(c3_sel), "r"(eip));
}

int cmpt = 0;

void interrupt_clock(int_ctx_t* ctx) {
  debug("Been interrupted by clock ! (%d)\n", cmpt);
  cmpt++;
  if (cmpt == 1)
    init_userland_1();
  else if(cmpt == 2)
    user2();
    //init_userland_2();
  else if (cmpt % 2 != 0)
    // user1();
    enter_userland_1(ctx->esp.blow, ctx->eip.blow);
  else
    user2();
    //enter_userland_2(ctx->esp);
}

void tp() {
  init_gdt();
  intr_init();

  register_gate(32, &interrupt_clock);
  force_interrupts_on();
  while (1) {
  }
}
