/* GPLv2 (c) Airbus */
#include <debug.h>
#include <info.h>
#include <cr.h>
#include <pagemem.h>

extern info_t *info;

void display_cr3() {
  cr3_reg_t cr3;
  cr3.raw = get_cr3();

  debug("CR3: %p\n", cr3);
}

void display_cr0() {
  cr0_reg_t cr0;
  cr0.raw = get_cr0();

  debug("CR0: %p\n", cr0);
}

void tp()
{
  pde32_t* PGD = (pde32_t*)0x600000;
  pde32_t* PGD = (pde32_t*)0x601000;

  uint32_t pagination_flag = 0x1 << 31;

  cr0_reg_t cr0;
  cr0.raw = get_cr0() | pagination_flag;

  debug("PG flag: %p\n", pagination_flag);

  set_cr3((uint32_t)PGD);
  set_cr0((uint32_t)cr0.raw);

  display_cr3();
  display_cr0();
}
