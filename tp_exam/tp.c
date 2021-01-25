/* GPLv2 (c) Airbus */
#include <asm.h>
#include <debug.h>
#include <info.h>
#include <intr.h>

#include "xsegmentation.h"

extern info_t* info;

seg_desc_t GDT[6];
tss_t TSS;

int task_switch_cmpt = 0;
int_ctx_t* user1_ctx;
int_ctx_t* user2_ctx;

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
    debug("running task 1 ...\n");
  }
  // asm volatile("mov %eax, %cr0"); // fail if we are in ring3
}

void user2() {
  debug("START TASK 2\n");
  while (1) {
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

  asm volatile(
      "push %0 \n"  // ss
      "push %1 \n"  // esp
      "pushf   \n"  // eflags
      "push %2 \n"  // cs
      "push %3 \n"  // eip
      "iret" ::"i"(d3_sel),
      "m"(ustack), "i"(c3_sel), "r"(&user1));
}

void enter_userland_1(uint32_t eip, uint32_t esp, uint32_t ebp) {
  debug("ENTER TASK 1\n");
  set_ds(d3_sel);
  set_es(d3_sel);
  set_fs(d3_sel);
  set_gs(d3_sel);

  TSS.s0.esp = get_ebp();
  TSS.s0.ss = d0_sel;

  asm volatile(
      "push %0 \n"        // ss
      "push %1 \n"        // esp
      "pushf   \n"        // eflags
      "push %2 \n"        // cs
      "push %3 \n"        // eip
      "mov %4, %%ebp \n"  // ebp
      "iret" ::"i"(d3_sel),
      "m"(esp), "i"(c3_sel), "r"(eip), "r"(ebp));
}

void save_userland_1(int_ctx_t* ctx) { user1_ctx = ctx; }
int_ctx_t* restore_userland_1() { return user1_ctx; }

void interrupt_clock(int_ctx_t* ctx) {
  debug("Been interrupted by clock ! (%d)\n", task_switch_cmpt);
  task_switch_cmpt++;
  force_interrupts_on();
  if (task_switch_cmpt == 1) {
    init_userland_1();
  } else if (task_switch_cmpt == 2) {
    save_userland_1(ctx);
    // init_userland_2();
    user2();
  } else if (task_switch_cmpt % 2 != 0) {
    int_ctx_t* task_ctx = restore_userland_1();
    debug("ESP: %x\n", task_ctx->esp.raw);
    debug("EBP: %x\n", task_ctx->gpr.ebp);
    debug("EIP: %x\n", task_ctx->eip.raw);
    enter_userland_1(task_ctx->eip.raw, task_ctx->esp.raw,
                     task_ctx->gpr.ebp.raw);
  } else {
    // save_userland_1(ctx);
    // ctx = restore_userland_2();
    // enter_userland_2(ctx->esp, ctx->eip);
    user2();
  }
}

void tp() {
  init_gdt();
  intr_init();

  register_gate(32, &interrupt_clock);
  force_interrupts_on();
  while (1) {
  }
}
