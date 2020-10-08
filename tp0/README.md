# TP0 - Familiarisation avec le noyau secos

Le but du TP est de se familiariser avec le noyau secos.

Le noyau est linké grâce au LD-script "utils/linker.lds" qui définit l'agencement mémoire du noyau lorsqu'il va être chargé par le bootloader GRUB.

```bash
(tp0)$ readelf -l kernel.elf

Elf file type is EXEC (Executable file)
Entry point 0x302010
There are 3 program headers, starting at offset 52

Program Headers:
  Type           Offset   VirtAddr   PhysAddr   FileSiz MemSiz  Flg Align
  LOAD           0x000094 0x00300000 0x00300000 0x0000c 0x0000c RWE 0x4
  LOAD           0x0000a0 0x00300010 0x00300010 0x00000 0x02000 RW  0x10
  LOAD           0x0000b0 0x00302010 0x00302010 0x013f8 0x01810 RWE 0x20
```

Le fichier ELF nous indique que le noyau est chargé à l'adresse physique `0x300000`.

Lors du démarrage, le noyau vous affiche sur le port série, la zone mémoire qu'il occupe:

```bash
(tp0)$ make qemu
secos-a241db6-59e4545 (c) Airbus
kernel mem [0x302010 - 0x303820]
```

Si vous regardez le code "start.c", vous découvrirez l'affichage de ces informations à l'aide des symbols `__kernel_start__` et `__kernel_end__`.

Vous pouvez modifier le fichier "tp0/tp.c" pour commencer le TP.

Par défaut, les fichiers de TP permettent d'accéder à un objet global pré-initialisé `info`. C'est un pointeur dont la structure est définie dans "include/info.h". Pour l'instant il ne contient que le champ `mbi` provenant de Grub. Ce champ donne accès à la structure `multiboot_info` (version 1) qui contient de nombreuses informations sur le système. Vous trouverez sa définition dans "include/mbi.h" et "include/grub_mbi.h".


## Questions

### Question 1

**Pourquoi le noyau indique `0x302010` et pas `0x300000` comme adresse de début ? Indice: essayer de comprendre linker.lds, regardez également le code de "entry.s"**

Dans le fichier `linker.ld`,
l'adresse est définie à `0x300000` en ligne 16 et la ligne 20 stoque la variable
`__kernel_start__` qui sera ensuite affichée par le fichier `tp.c`. Cependant,
les lignes 17 et 18 augmentent encore de `0x2010` l'adresse de départ :

#### MBH (MultiBoot Headers)

La ligne 17 récupère la sections `.mbh` définie dans `start.c`.
```bash
$ readelf -s start.o | grep mbh
    16: 00000000    12 OBJECT  GLOBAL DEFAULT    6 mbh
$ grep -r __mbh__ kernel/
kernel/core/start.c:volatile const uint32_t __mbh__ mbh[] = {
kernel/include/mbi.h:#define __mbh__ __attribute__ ((section(".mbh"),aligned(4)))
```

On constate que dans le fichier de sortie, l'objet mbh est bien en première
position dans le noyau.
```bash
$ readelf -s kernel.elf | grep 300000
     1: 00300000     0 SECTION LOCAL  DEFAULT    1 
    53: 00300000    12 OBJECT  GLOBAL DEFAULT    1 mbh
```

La section `.mbh` contient un objet `mbh` de "type" `__mbh__` défini par une
macro à la ligne 25 du fichier `include/mbi.h` :
```c
	#define __mbh__ __attribute__ ((section(".mbh"),aligned(4)))
```

L'objet est initialisé dans le fichier `core/start.c`:
```c
volatile const uint32_t __mbh__ mbh[] = {
   MBH_MAGIC,													// définis l'espace comme bootable
   MBH_FLAGS,													// flags pour GRUB : comment load kernel
   (uint32_t)-(MBH_MAGIC+MBH_FLAGS),	// checksum du multiboot_header
};
```
( il est sensé faire `3 * 4 = 12` octets, pourquoi il n'en prend que 10 ?? )

#### La stack

Vient ensuite la section `.stack` dans le fichier `linker.lds`. Cette section
est définis par les lignes 2 à 4 du fichier `core/entry.s`:
```asm
.section .stack, "aw", @nobits
.align 16
.space 0x2000
```
On vois qu'une section `stack` y est déclarée et que 2000 octets lui sont
alloués.

---

### Question 2

**A l'aide de la structure "multiboot_info", vous devez afficher la cartographie mémoire de la VM. Pour cela, utilisez les champs `mmap_addr` et `mmap_length`. Aidez-vous d'internet pour trouver des informations sur le standard multiboot. Le champ `mmap_addr` contient l'adresse du premier objet de type `multiboot_memory_map` qui vous permettra d'afficher des informations sur la mémoire.**

```
################
start   : 0x0
end     : 0x9fc00
size    : 654336
state   : available
################
start   : 0x9fc00
end     : 0xa0000
size    : 1024
state   : reserved
################
start   : 0xf0000
end     : 0x100000
size    : 65536
state   : reserved
################
start   : 0x100000
end     : 0x7fe0000
size    : 133038080
state   : available
################
start   : 0x7fe0000
end     : 0x8000000
size    : 131072
state   : reserved
################
start   : 0xfffc0000
end     : 0x0
size    : 262144
state   : reserved
```

---

### Question 3

**Vous allez découvrir différentes zones de mémoire physique, certaines étant réservées, d'autres libres. Déclarez un pointeur d'entier par exemple et initialisez le avec une des adresses que vous avez trouvée. Essayez de lire/écrire la mémoire à cette adresse. Que se passe-t-il ? Avez-vous un "segmentation fault" ? Pourquoi ?**

---

### Question 4

**Essayez de lire/écrire en dehors de la RAM disponible (128MB). Que se passe-t-il ?**
