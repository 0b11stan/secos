/* GPLv2 (c) Airbus */
#include <debug.h>
#include <info.h>

#include "intr.h"

extern info_t *info;


void interrupt_handler(int_ctx_t *ctx)
{
  asm volatile ("pusha");
  debug("bp triggered : %p\n", ctx);
  // clean the context pushed by current function
  // return from interruption
  asm volatile ("popa ; leave ; iret");
}

void bp_trigger()
{
  asm volatile ("int3");
  debug("continue execution...\n");
}

void tp()
{
  int_desc_t *IDT;

  idt_reg_t idtr;

  idtr.limit = 0;
  idtr.addr = 0;
  idtr.desc = 0;

  get_idtr(idtr);

  IDT = idtr.desc;

  int_desc(&IDT[3], gdt_krn_seg_sel(1), (offset_t)interrupt_handler);

  bp_trigger();

  debug("This is a working bp handler !\n");
}
