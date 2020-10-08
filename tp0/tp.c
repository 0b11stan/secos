/* GPLv2 (c) Airbus */
#include <debug.h>
#include <info.h>

extern info_t   *info;
extern uint32_t __kernel_start__; // vient du fichier `linker.lds`
extern uint32_t __kernel_end__;		// vient du fichier `linker.lds`

char *get_type(multiboot_uint32_t type) {
	switch (type) {
		case MULTIBOOT_MEMORY_AVAILABLE:
			return "available"; break;
		case MULTIBOOT_MEMORY_RESERVED:
			return "reserved"; break;
		case MULTIBOOT_MEMORY_ACPI_RECLAIMABLE:
			return "reserved (acpi)"; break;
		case MULTIBOOT_MEMORY_NVS:
			return "reserved (nvs)"; break;
		default:
			return "?";
	}
}

void tp() {
   debug("kernel mem [0x%x - 0x%x]\n", &__kernel_start__, &__kernel_end__);
   debug("MBI flags 0x%x\n", info->mbi->flags);

	 uint32_t last_mmap_addr = info->mbi->mmap_addr + info->mbi->mmap_length;
	 multiboot_memory_map_t *current_entry = (multiboot_memory_map_t*)info->mbi->mmap_addr;
	 debug("The memory map goes from 0x%x to 0x%x\n", info->mbi->mmap_addr,
			 last_mmap_addr);

	 while ((uint32_t)current_entry < last_mmap_addr) {
		 debug("################\n");
		 //debug("size    : %u\n", current_entry->size);
		 debug("start   : 0x%x\n", current_entry->addr);
		 debug("end     : 0x%x\n", current_entry->addr + current_entry->len);
		 debug("size    : %u\n", current_entry->len);
		 debug("state   : %s\n", get_type(current_entry->type));

		 current_entry += 1;
	 }
}
