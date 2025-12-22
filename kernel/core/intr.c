/* GPLv2 (c) Airbus */
#include <intr.h>
#include <debug.h>
#include <info.h>
#include <segmem.h>
#include <grub_mbi.h>

#include <task.h>
#include <configuration_files/tss/tss_setup.h>

extern uint32_t __kernel_stack_user1_base__, __kernel_stack_user1_end__;
extern uint32_t __kernel_stack_user2_base__, __kernel_stack_user2_end__;

uint32_t k1_base = (uint32_t)&__kernel_stack_user1_base__;
uint32_t k1_end  = (uint32_t)&__kernel_stack_user1_end__;

uint32_t k2_base = (uint32_t)&__kernel_stack_user2_base__;
uint32_t k2_end  = (uint32_t)&__kernel_stack_user2_end__;

extern info_t *info;
extern void idt_trampoline();
static int_desc_t IDT[IDT_NR_DESC];

void intr_init()
{
   idt_reg_t idtr;
   offset_t  isr;
   size_t    i;

   isr = (offset_t)idt_trampoline;

   /* re-use default grub GDT code descriptor */
   for(i=0 ; i<IDT_NR_DESC ; i++, isr += IDT_ISR_ALGN)
      build_int_desc(&IDT[i], gdt_krn_seg_sel(1), isr);

   // syscall : les users en ring3 doivent pouvoir faire int 0x80
   IDT[0x80].dpl = 3;
   IDT[0x80].p  = 1;

   idtr.desc  = IDT;
   idtr.limit = sizeof(IDT) - 1;
   set_idtr(idtr);
}

uint32_t __regparm__(1) intr_hdlr(int_ctx_t *ctx)
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

   uint8_t vector = ctx->nr.blow;

   /* Exceptions generales du CPU */
   if (vector < NR_EXCP) {
   excp_hdlr(ctx);
   return (uint32_t)ctx;
   }

   /* Syscall 80 */
   if (vector == 0x80) {
      // TODO: syscall_hdlr(ctx); (tu l’ajouteras)
      return (uint32_t)ctx;
   }

   // Gestion de irq0
   if(vector == 32){

      // cpl au moment de l'interruption
      uint32_t cpl = ctx->cs.raw & 3;

      // esp de la pile noyau courant
      uint32_t kernel_esp = (uint32_t)ctx;

      debug("interruption 32 \n");

      if(cpl == 3){
         debug("l'interruption a ete declanchee par un user");

         if (current_task == &task_user1) {

            debug("IRQ sur pile noyau USER1 (kesp=0x%x)\n", kernel_esp);
            debug("User 1 etait en cours au moment de l'interruption irq0\n");
            
            // On sauvegarde le pointeur de pile kernel de user 1 dans sa struct
            task_user1.kernel_esp = (uint32_t)ctx;

            // Switch la tache courante
            current_task = &task_user2;

            // Switch CR3
            set_cr3(&task_user2.cr3);

            // Mise a jour TSS avec bonne pile kernel pour la prochaine interruption
            TSS.s0.esp = __kernel_stack_user2_end__; // adresse 0x00c0 2000
            TSS.s0.ss  = gdt_krn_seg_sel(3);

            //On envoit le nouveau esp pile user 2 a idt.s
            return &task_user2.kernel_esp;

            
         } 
         else if (current_task == &task_user2) {

            debug("IRQ sur pile noyau USER2 (kesp=0x%x)\n", kernel_esp);
            debug("User 2 etait en cours au moment de l'interruption irq0\n");

            // On sauvegarde le pointeur de pile kernel de user 1 dans sa struct
            task_user2.kernel_esp = (uint32_t)ctx;

            // Switch la tache courante
            current_task = &task_user1;

            // Switch CR3
            set_cr3(&task_user1.cr3);

            // Mise a jour TSS avec bonne pile kernel pour la prochaine interruption
            TSS.s0.esp = __kernel_stack_user1_end__; // adresse 0x00c0 2000
            TSS.s0.ss  = gdt_krn_seg_sel(2);

            //On envoit le nouveau esp pile user 1 a idt.s
            return &task_user1.kernel_esp;
         } 
         else {
            debug("IRQ sur pile noyau inconnue (kesp=0x%x)\n", kernel_esp);
            debug("Erreur lors de la recuperation du user en cours au moment de l'irq0");
         }

      }else{
         debug("l'interruption a ete declanchee par le noyau");
      }
   }else{
      debug("l'interruption declanchee n'est pas l'interruption 32\n");
   }
}
