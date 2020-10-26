/* GPLv2 (c) Airbus */
#include <intr.h>
#include <debug.h>
#include <info.h>

extern info_t *info;
extern void idt_trampoline();       // définis dans idt.s
static int_desc_t IDT[IDT_NR_DESC]; // l'IDT (tableau de descripteur)

void intr_init()
{
   idt_reg_t idtr; // registre contenant l'addresse de l'IDT
   offset_t  isr;  // contiendra les addresses de chaque interruption dans idt.s
   size_t    i;

   isr = (offset_t)idt_trampoline; // pointe vers la première interruption

   /*
    * IDT_NR_DESC  256 nombre max de descripteur dans l'IDT
    * IDT_ISR_ALGN 16  taille qui concorde avec les mention `.align 16` dans
    *                  le fichier idt.s
    * L'instruction `isr += IDT_ISR_ALGN` est jouée à chaque itération et pointe
    * ainsi la première instruction de `l'appel de fonction` qui réagit à une
    * intéruption donnée.
    */
   for(i=0 ; i<IDT_NR_DESC ; i++, isr += IDT_ISR_ALGN)
      int_desc(&IDT[i], gdt_krn_seg_sel(1), isr);
      // gdt_krn_seg_sel(1) = default grub GDT code descriptor 

   idtr.desc  = IDT;             // premier descripteur d'interruption
   idtr.limit = sizeof(IDT) - 1; // la taille de l'IDT en octets
   set_idtr(idtr);
}

/**
 * Affiche l'état de tout les registres et flags (contexte d'execution quoi)
 * Est appelé dans idt.s après l'étiquette `idt_common`:
 * - Le pusha à mis tout les registres dans la stack
 * - Le mov donne comme argument au call l'adresse actuelle de la stack
 **/
void __regparm__(1) intr_hdlr(int_ctx_t *ctx)
{
   debug("\nIDT event\n"
         " . int    #%d\n"
         " . error  0x%x\n"
         " . cs:eip 0x%x:0x%x\n"
         " . ss:esp 0x%x:0x%x\n"
         " . eflags 0x%x\n"
         "\n- GPR\n"
         "eax     : 0x%x\n"
         "ecx     : 0x%x\n"
         "edx     : 0x%x\n"
         "ebx     : 0x%x\n"
         "esp     : 0x%x\n"
         "ebp     : 0x%x\n"
         "esi     : 0x%x\n"
         "edi     : 0x%x\n"
         ,ctx->nr.raw, ctx->err.raw
         ,ctx->cs.raw, ctx->eip.raw
         ,ctx->ss.raw, ctx->esp.raw
         ,ctx->eflags.raw
         ,ctx->gpr.eax.raw
         ,ctx->gpr.ecx.raw
         ,ctx->gpr.edx.raw
         ,ctx->gpr.ebx.raw
         ,ctx->gpr.esp.raw
         ,ctx->gpr.ebp.raw
         ,ctx->gpr.esi.raw
         ,ctx->gpr.edi.raw);

   uint8_t vector = ctx->nr.blow; // low byte (dernier octet de nr)
                                  // nr pour number ?? c'est l'"identifiant"
                                  // de l'interruption qui à été appelé. On peut
                                  // le caster sur un u8 parcequ'on sait qu'il
                                  // n'y aura pas plus de 256 interruptions (cf.
                                  // IDT_NR_DESC)

   if(vector < NR_EXCP) // traite les exception qui sont toutes les
      excp_hdlr(ctx);   // interruptions avant `pic 1` (donc < 32 logic)
   else
      debug("ignore IRQ %d\n", vector); // l'interruption à été ignorée
      // c'est ici qu'on pourra mettre un bon gros switch pour traiter les IRT
}
