/* GPLv2 (c) Airbus */
#include <asm.h>
#include <cr.h>
#include <debug.h>
#include <info.h>
#include <intr.h>
#include <pagemem.h>

#include "xsegmentation.h"

#define PGD_KERNEL 0x400000
#define PTB_KERNEL 0x401000

#define PGD_TASK1 0x403000
#define PTB_TASK1 0x404000

#define PGD_TASK2 0x406000
#define PTB_TASK2 0x407000

#define STACK_TASK1 0x500000
#define KERNEL_STACK_TASK1 0x501000
#define STACK_TASK2 0x502000
#define KERNEL_STACK_TASK2 0x503000
#define SHARED_MEM 0x504000

extern info_t* info;

typedef struct task {
  uint16_t cs;
  uint16_t ds;
  uint32_t pgd;
  uint32_t tss;
  uint32_t eip;
  gpr_ctx_t gpr;
} task_t;

seg_desc_t GDT[6];
tss_t TSS_KERNEL;
tss_t TSS_KERNEL_TASK1;
tss_t TSS_KERNEL_TASK2;

pde32_t* pgd_kernel;
int task_switch_cmpt = 0;

pde32_t* pgd_task1;
int_ctx_t* user1_ctx;

pde32_t* pgd_task2;
int_ctx_t* user2_ctx;

task_t task1;
task_t task2;

void init_gdt() {
  gdt_reg_t gdtr;

  GDT[0].raw = 0ULL;

  c0_dsc(&GDT[c0_idx]);
  d0_dsc(&GDT[d0_idx]);
  c3_dsc(&GDT[c3_idx]);
  d3_dsc(&GDT[d3_idx]);
  tss_dsc(&GDT[tsk0_idx], (offset_t)&TSS_KERNEL);
  tss_dsc(&GDT[tsk1_idx], (offset_t)&TSS_KERNEL_TASK1);
  tss_dsc(&GDT[tsk2_idx], (offset_t)&TSS_KERNEL_TASK2);

  gdtr.desc = GDT;
  gdtr.limit = sizeof(GDT) - 1;
  set_gdtr(gdtr);

  set_cs(c0_sel);
  set_ss(d0_sel);
  set_ds(d0_sel);
  set_es(d0_sel);
  set_fs(d0_sel);
  set_gs(d0_sel);
  set_tr(tsk0_sel);
}

void user1() {
  debug("START TASK 1\n");
  debug("kernel: %s\n", (char*)0x2000);  // TODO : should map so that fail
  char* message = "Hi from user1!";
  asm volatile("int $80" ::"S"(message));
  while (1) debug("running task ONE ...\n");
}

void user2() {
  debug("START TASK 2\n");
  debug("kernel: %s\n", (char*)0x2000);  // TODO : should map so that fail
  while (1) debug("running task TWO ...\n");
}

void enter_userland(task_t* task) {
  debug("ENTER USERLAND %p\n", task->pgd);

  set_ds(task->ds);
  set_es(task->ds);
  set_fs(task->ds);
  set_gs(task->ds);

  TSS_KERNEL.s0.esp = get_ebp();
  TSS_KERNEL.s0.ss = d0_sel;

  asm volatile(
      "push %0 \n"        // ss
      "push %1 \n"        // esp
      "pushf   \n"        // eflags
      "push %2 \n"        // cs
      "push %3 \n"        // eip
      "mov %4, %%cr3 \n"  // cr3
      "mov %5, %%ebp \n"  // ebp
      "iret" ::"r"((uint32_t)task->ds),
      "m"(task->gpr.esp.raw), "r"((uint32_t)task->cs), "r"(task->eip),
      "r"(task->pgd), "r"(task->gpr.ebp.raw));
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

void interrupt_syscall(int_ctx_t* ctx) {
  debug("Been interrupted by syscall !\n");
  debug("message: '%s'\n", ctx->gpr.esi);
}

void interrupt_clock(int_ctx_t* old) {
  debug("Been interrupted by clock ! (%d)\n", task_switch_cmpt);
  task_switch_cmpt++;

  force_interrupts_on();

  task_t* task;

  if (task_switch_cmpt == 1) {
    // init user1
    task = &task1;
  } else if (task_switch_cmpt == 2) {
    // init user2
    user1_ctx = old;
    task = &task2;
  } else {
    // switch tasks context
    int_ctx_t* new = switch_context(old);
    task = new == user1_ctx ? &task1 : &task2;
    task->eip = new->eip.raw;
    task->gpr.esp.raw = new->esp.raw;
    task->gpr.ebp.raw = new->gpr.ebp.raw;
  }
  // run task
  enter_userland(task);
}

pde32_t* init_pgd(uint32_t address) {
  memset((void*)address, 0, PAGE_SIZE);
  return (pde32_t*)address;
}

void enable_paging() {
  uint32_t cr0 = get_cr0();
  set_cr0(cr0 | CR0_PG);
}

void map_full_table(pde32_t* pde, uint32_t pte, uint32_t flags) {
  pte32_t* ptb = (pte32_t*)pte;
  for (int i = 0; i < 1024; i++) pg_set_entry(&ptb[i], flags, i);
  pg_set_entry(pde, flags, page_nr(ptb));
}

void map_user_page(pde32_t* pde, uint32_t pte, uint32_t index, uint32_t stack) {
  pte32_t* ptb = (pte32_t*)pte;
  pg_set_entry(&ptb[index], PG_USR | PG_RW, page_nr(stack));
  pg_set_entry(pde, PG_USR | PG_RW, page_nr(ptb));
}

void init_pagination() {
  // TODO : only map what's needed
  // TODO : map only kernel stack with access to ring0
  // TODO : map only kernel code with access to ring3
  pgd_kernel = init_pgd(PGD_KERNEL);
  pgd_task1 = init_pgd(PGD_TASK1);
  pgd_task2 = init_pgd(PGD_TASK2);

  // kernel
  map_full_table(&pgd_kernel[0], PTB_KERNEL, PG_KRN | PG_RW);
  map_full_table(&pgd_kernel[1], (PTB_KERNEL + 0x1000), PG_KRN | PG_RW);

  // task1
  // map_full_table(&pgd_task1[0], PTB_TASK1, PG_KRN | PG_RW);
  map_full_table(&pgd_task1[0], PTB_TASK1, PG_USR | PG_RW);
  map_user_page(&pgd_task1[1], (PTB_TASK1 + 0x1000), 256, STACK_TASK1);
  // map_user_page(&pgd_task1[1], (PTB_TASK1 + 0x1000), 258, SHARED_MEM);

  // task2
  // map_full_table(&pgd_task2[0], PTB_TASK2, PG_KRN | PG_RW);
  map_full_table(&pgd_task2[0], PTB_TASK2, PG_USR | PG_RW);
  map_user_page(&pgd_task2[1], (PTB_TASK2 + 0x1000), 258, STACK_TASK2);
  // map_user_page(&pgd_task2[1], (PTB_TASK2 + 0x1000), 258, SHARED_MEM);

  set_cr3((uint32_t)pgd_kernel);
  enable_paging();
}

void init_syscall() {
  idt_reg_t idtr;
  get_idtr(idtr);
  int_desc_t* dsc = &idtr.desc[80];
  dsc->dpl = 3;
}

void tp() {
  init_gdt();
  intr_init();
  init_pagination();
  init_syscall();

  register_gate(80, &interrupt_syscall);
  register_gate(32, &interrupt_clock);

  memset(&task1, 0, sizeof(task_t));
  task1.cs = c3_sel;
  task1.ds = d3_sel;
  task1.pgd = PGD_TASK1;
  task1.tss = KERNEL_STACK_TASK1 + 0xfff;
  task1.eip = (uint32_t)&user1;
  task1.gpr.ebp.raw = STACK_TASK1 + 0xfff;
  task1.gpr.esp.raw = STACK_TASK1 + 0xfff;

  memset(&task2, 0, sizeof(task_t));
  task2.cs = c3_sel;
  task2.ds = d3_sel;
  task2.pgd = PGD_TASK2;
  task2.tss = KERNEL_STACK_TASK2 + 0xfff;
  task2.eip = (uint32_t)&user2;
  task2.gpr.ebp.raw = STACK_TASK2 + 0xfff;
  task2.gpr.esp.raw = STACK_TASK2 + 0xfff;

  force_interrupts_on();

  while (1)
    ;
}
