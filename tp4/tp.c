/* GPLv2 (c) Airbus */
#include <cr.h>
#include <debug.h>
#include <info.h>
#include <pagemem.h>

extern info_t* info;

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

/*
void page_map(uint32_t index, pte32_t* ptb) {
  for (int i = 0; i < 1024; i++)
    pg_set_entry(&ptb[i], PG_KRN | PG_RW, i + (1024 * index));
  pg_set_entry(&pgd[index], PG_KRN | PG_RW, page_nr(ptb));
}
*/

void tp() {
  int i;

  pde32_t* pgd = (pde32_t*)0x600000;
  memset((void*)pgd, 0, PAGE_SIZE);

  // map le code du noyau
  pte32_t* ptb1 = (pte32_t*)0x601000;
  for (i = 0; i < 1024; i++) pg_set_entry(&ptb1[i], PG_KRN | PG_RW, i + 1024*0);
  pg_set_entry(&pgd[0], PG_KRN | PG_RW, page_nr(ptb1));

  // map les tables de page
  pte32_t* ptb2 = (pte32_t*)0x602000;
  for (i = 0; i < 1024; i++) pg_set_entry(&ptb2[i], PG_KRN | PG_RW, i + 1024*1);
  pg_set_entry(&pgd[1], PG_KRN | PG_RW, page_nr(ptb2));

  
  // sauvegarde l'adresse du pgd
  set_cr3((uint32_t)pgd);

  // enable pagination
  cr0_reg_t cr0;
  uint32_t pagination_flag = 0x1 << 31;
  cr0.raw = get_cr0() | pagination_flag;
  set_cr0((uint32_t)cr0.raw);

 
   pte32_t  *ptb3    = (pte32_t*)0x603000;
   uint32_t *target  = (uint32_t*)0xc0000000;
   int      pgd_idx = pd32_idx(target);
   int      ptb_idx = pt32_idx(target);

   memset((void*)ptb3, 0, PAGE_SIZE);
   pg_set_entry(&ptb3[ptb_idx], PG_KRN|PG_RW, page_nr(pgd));
   pg_set_entry(&pgd[pgd_idx], PG_KRN|PG_RW, page_nr(ptb3));

   debug("PGD[0] = %p | target = %p\n", pgd[0].raw, *target);

   char *v1 = (char*)0x700000;
   char *v2 = (char*)0x7ff000;

   ptb_idx = pt32_idx(v1);
   pg_set_entry(&ptb2[ptb_idx], PG_KRN|PG_RW, 2);

   ptb_idx = pt32_idx(v2);
   pg_set_entry(&ptb2[ptb_idx], PG_KRN|PG_RW, 2);

   debug("%p = %s | %p = %s\n", v1, v1, v2, v2);

 // 9 : il faut également vider les TLBs
   *target = 0;
   invalidate(target);


  display_cr3();
  display_cr0();
  debug("PGD 1: %p\n", pgd[0x300].addr);
  debug("PTB: %p\n", ptb3[0x300].addr);

  }
