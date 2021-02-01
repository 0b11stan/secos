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
  uint32_t esp;
  uint32_t ebp;
} task_t;

seg_desc_t GDT[6];
tss_t TSS;

pde32_t* pgd_kernel;
pde32_t* pgd_task1;
pde32_t* pgd_task2;

task_t task1;
task_t task2;

void init_gdt() {
  gdt_reg_t gdtr;

  GDT[0].raw = 0ULL;

  c0_dsc(&GDT[c0_idx]);
  d0_dsc(&GDT[d0_idx]);
  c3_dsc(&GDT[c3_idx]);
  d3_dsc(&GDT[d3_idx]);
  tss_dsc(&GDT[tss_idx], (offset_t)&TSS);

  gdtr.desc = GDT;
  gdtr.limit = sizeof(GDT) - 1;
  set_gdtr(gdtr);

  set_cs(c0_sel);
  set_ss(d0_sel);
  set_ds(d0_sel);
  set_es(d0_sel);
  set_fs(d0_sel);
  set_gs(d0_sel);
  set_tr(tss_sel);

  TSS.s0.ss = d0_sel;
}

void user1() {
  uint32_t* counter = (uint32_t*)SHARED_MEM;
  while (1) (*counter)++;
}

void user2() {
  uint32_t* counter = (uint32_t*)SHARED_MEM;
  while (1) asm volatile("int $80" ::"S"(*counter));
}

void enter_userland(task_t* task) {
  debug("ENTER USERLAND %p\n", task->pgd);

  set_ds(task->ds);
  set_es(task->ds);
  set_fs(task->ds);
  set_gs(task->ds);

  /* TODO
  TSS.s0.esp = task->tss;
  TSS.gpr.ebp.raw = task->tss;
  */

  TSS.s0.esp = get_ebp();

  asm volatile(
      "push %0 \n"        // ss
      "push %1 \n"        // esp
      "pushf   \n"        // eflags
      "push %2 \n"        // cs
      "push %3 \n"        // eip
      "mov %4, %%cr3 \n"  // cr3
      "mov %5, %%ebp \n"  // ebp
      "iret" ::"r"((uint32_t)task->ds),
      "m"(task->esp), "r"((uint32_t)task->cs), "r"(task->eip), "r"(task->pgd),
      "r"(task->ebp));
}

short is_task_1(int_ctx_t* ctx) {
  uint32_t sp = ctx->esp.raw;
  return ((sp > STACK_TASK1) && (sp < STACK_TASK1 + 0xfff)) ||
         ((sp > KERNEL_STACK_TASK1) && (sp < KERNEL_STACK_TASK1 + 0xfff));
}

void store_task(task_t* task, int_ctx_t* ctx) {
  task->eip = ctx->eip.raw;
  task->esp = ctx->esp.raw;
  task->ebp = ctx->gpr.ebp.raw;
}

task_t* switch_context(int_ctx_t* old) {
  if (is_task_1(old)) {
    store_task(&task1, old);
    return &task2;
  } else {
    store_task(&task2, old);
    return &task1;
  }
}

void interrupt_syscall(int_ctx_t* ctx) {
  // debug("Been interrupted by syscall !\n");
  debug("counter: '%d'\n", ctx->gpr.esi);
}

void interrupt_clock(int_ctx_t* old) {
  debug("Been interrupted by clock !\n");
  force_interrupts_on();
  task_t* task = switch_context(old);
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

void map_user_page(pde32_t* pde, uint32_t pte, uint32_t index, uint32_t addr) {
  pte32_t* ptb = (pte32_t*)pte;
  pg_set_entry(&ptb[index], PG_USR | PG_RW, page_nr(addr));
  pg_set_entry(pde, PG_USR | PG_RW, page_nr(ptb));
}

void init_pagination() {
  pgd_kernel = init_pgd(PGD_KERNEL);
  pgd_task1 = init_pgd(PGD_TASK1);
  pgd_task2 = init_pgd(PGD_TASK2);

  // kernel
  map_full_table(&pgd_kernel[0], PTB_KERNEL, PG_KRN | PG_RW);
  map_full_table(&pgd_kernel[1], (PTB_KERNEL + 0x1000), PG_KRN | PG_RW);

  // task1
  map_full_table(&pgd_task1[0], PTB_TASK1, PG_USR | PG_RO);
  map_user_page(&pgd_task1[1], (PTB_TASK1 + 0x1000), 256, STACK_TASK1);
  map_user_page(&pgd_task1[1], (PTB_TASK1 + 0x1000), 260, SHARED_MEM);

  // task2
  map_full_table(&pgd_task2[0], PTB_TASK2, PG_USR | PG_RO);
  map_user_page(&pgd_task2[1], (PTB_TASK2 + 0x1000), 258, STACK_TASK2);
  map_user_page(&pgd_task2[1], (PTB_TASK2 + 0x1000), 260, SHARED_MEM);

  set_cr3((uint32_t)pgd_kernel);
  enable_paging();
}

void init_syscall() {
  idt_reg_t idtr;
  get_idtr(idtr);
  int_desc_t* dsc = &idtr.desc[80];
  dsc->dpl = 3;
}

void init_task(task_t* task, uint32_t pgd, uint32_t stack,
               uint32_t kernel_stack, void (*routine)()) {
  memset(task, 0, sizeof(task_t));
  task->cs = c3_sel;
  task->ds = d3_sel;
  task->pgd = pgd;
  task->tss = kernel_stack + 0xfff;
  task->eip = (uint32_t)routine;
  task->ebp = stack + 0xfff;
  task->esp = stack + 0xfff;
}

void tp() {
  init_gdt();
  intr_init();
  init_pagination();
  init_syscall();

  memset((int*)SHARED_MEM, 0, 32);

  register_gate(80, &interrupt_syscall);
  register_gate(32, &interrupt_clock);

  init_task(&task1, PGD_TASK1, STACK_TASK1, KERNEL_STACK_TASK1, &user1);
  init_task(&task2, PGD_TASK2, STACK_TASK2, KERNEL_STACK_TASK2, &user2);

  force_interrupts_on();
  enter_userland(&task1);
}
