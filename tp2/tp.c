/* GPLv2 (c) Airbus */
#include <debug.h>
#include <info.h>

#include "intr.h"

extern info_t *info;

void tp()
{
  while(1) {
    asm volatile("sti");
  }
}
