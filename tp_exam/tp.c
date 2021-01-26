/* GPLv2 (c) Airbus */
#include <asm.h>
#include <debug.h>
#include <info.h>
#include <intr.h>

#include "xsegmentation.h"

#define STACK_TASK1 0x600000
#define STACK_TASK2 0x601000

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
  while (1) debug("running task 1 ...\n");
}

void user2() {
  debug("START TASK 2\n");
  while (1) debug("running task 2 ...\n");
}

void enter_userland(uint32_t eip, uint32_t esp, uint32_t ebp) {
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

int_ctx_t* switch_context(int_ctx_t* old) {
  if (task_switch_cmpt % 2 != 0) {
    // run user1
    user2_ctx = old;
    return user1_ctx;
  } else {
    // run user2
    user1_ctx = old;
    return user2_ctx;
  }
}

void interrupt_clock(int_ctx_t* old) {
  debug("Been interrupted by clock ! (%d)\n", task_switch_cmpt);
  uint32_t teip = 0;
  uint32_t tesp = 0;
  uint32_t tebp = 0;

  task_switch_cmpt++;
  force_interrupts_on();

  if (task_switch_cmpt == 1) {
    // init user1
    teip = (uint32_t)&user1;
    tesp = STACK_TASK1;
    tebp = STACK_TASK1;
  } else if (task_switch_cmpt == 2) {
    // init user2
    user1_ctx = old;
    teip = (uint32_t)&user2;
    tesp = STACK_TASK2;
    tebp = STACK_TASK2;
  } else {
    // switch and run
    int_ctx_t* new = switch_context(old);
    teip = new->eip.raw;
    tesp = new->esp.raw;
    tebp = new->gpr.ebp.raw;
  }
  enter_userland(teip, tesp, tebp);
}

void tp() {
  init_gdt();
  intr_init();

  register_gate(32, &interrupt_clock);
  force_interrupts_on();

  while (1)
    ;
}
