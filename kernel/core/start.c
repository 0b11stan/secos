/* GPLv2 (c) Airbus */
#include <start.h>
#include <debug.h>
#include <pic.h>
#include <uart.h>
#include <intr.h>
#include <info.h>

// source https://intermezzos.github.io/book/first-edition/multiboot-headers.html
volatile const uint32_t __mbh__ mbh[] = {
   MBH_MAGIC,													// définis l'espace comme bootable
   MBH_FLAGS,													// flags pour GRUB : comment load kernel
   (uint32_t)-(MBH_MAGIC+MBH_FLAGS),	// checksum du multiboot_header
};

static info_t __info __attribute__ ((aligned(16)));
       info_t *info = &__info;

void __attribute__((regparm(1))) start(mbi_t *mbi)
{
   info->mbi = mbi;

   pic_init();
   uart_init();
   intr_init();
   debug("\n" RELEASE " (c) Airbus\n");

   tp();

   panic("halted !");
}
