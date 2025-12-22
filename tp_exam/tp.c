/* GPLv2 (c) Airbus */
#include <debug.h>
#include <info.h>
#include <segmem.h>
#include <string.h>
#include <pagemem.h>
#include <cr.h>
#include <intr.h>
#include <pic.h>

//------------------------------------------------------------- MEMOIRE -------------------------------------------------------------
extern info_t   *info;
extern uint32_t __kernel_start__;
extern uint32_t __kernel_end__;

char* inttypetochar(int t) {
   switch(t) {

      case 1:
         return("MULTIBOOT_MEMORY_AVAILABLE");
      case 2:
         return("MULTIBOOT_MEMORY_RESERVED");
      case 3:
         return("MULTIBOOT_MEMORY_ACPI_RECLAIMABLE");
      case 4:
         return("MULTIBOOT_MEMORY_NVS");
      default:
         return("Unknown type");
   }
}

void print_mem(){
   debug("kernel mem [0x%p - 0x%p]\n", &__kernel_start__, &__kernel_end__);
   debug("MBI flags 0x%x\n", info->mbi->flags);

   multiboot_memory_map_t* entry = (multiboot_memory_map_t*)info->mbi->mmap_addr;
	while((uint32_t)entry < (info->mbi->mmap_addr + info->mbi->mmap_length)) {
        // Q2 
        debug("[0x%x - ", (unsigned int)entry->addr);
        debug("0x%x]", (unsigned int) (entry->len + entry->addr - 1));
        debug(" %s\n", inttypetochar(entry->type));
        // end Q2
        entry++;
   }
}




//------------------------------------------------------------- SEGMENTATION -------------------------------------------------------------




void print_gdt_content(gdt_reg_t gdtr_ptr) {
    seg_desc_t* gdt_ptr;
    gdt_ptr = (seg_desc_t*)(gdtr_ptr.addr);
    int i=0;
    while ((uint32_t)gdt_ptr < ((gdtr_ptr.addr) + gdtr_ptr.limit)) {
        uint32_t start = gdt_ptr->base_3<<24 | gdt_ptr->base_2<<16 | gdt_ptr->base_1;
        uint32_t end;
        if (gdt_ptr->g) {
            end = start + ( (gdt_ptr->limit_2<<16 | gdt_ptr->limit_1) <<12) + 4095;
        } else {
            end = start + (gdt_ptr->limit_2<<16 | gdt_ptr->limit_1);
        }
        debug("%d ", i);
        debug("[0x%x ", start);
        debug("- 0x%x] ", end);
        debug("seg_t: 0x%x ", gdt_ptr->type);
        debug("desc_t: %d ", gdt_ptr->s);
        debug("priv: %d ", gdt_ptr->dpl);
        debug("present: %d ", gdt_ptr->p);
        debug("avl: %d ", gdt_ptr->avl);
        debug("longmode: %d ", gdt_ptr->l);
        debug("default: %d ", gdt_ptr->d);
        debug("gran: %d ", gdt_ptr->g);
        debug("\n");
        gdt_ptr++;
        i++;
    }
}

//Structures pour GDT
// Index dans la GDT
#define c0_idx  1
#define s0_t1_idx  2
#define s0_t2_idx  3
#define c3_t1_idx  4
#define c3_t2_idx  5
#define d3_t1_idx  6
#define d3_t2_idx  7
#define d3_idx  8
#define ts_idx  9
// Selecteurs
#define c0_sel  gdt_krn_seg_sel(c0_idx)
#define s0_t1_sel  gdt_krn_seg_sel(s0_t1_idx)
#define s0_t2_sel  gdt_krn_seg_sel(s0_t2_idx)
#define c3_t1_sel  gdt_krn_seg_sel(c3_t1_idx)
#define c3_t2_sel  gdt_krn_seg_sel(c3_t2_idx)
#define d3_t1_sel  gdt_krn_seg_sel(d3_t1_idx)
#define d3_t2_sel  gdt_krn_seg_sel(d3_t2_idx)
#define d3_sel  gdt_krn_seg_sel(d3_idx)
#define ts_sel  gdt_krn_seg_sel(ts_idx)
// Déclaration de notre GDT et notre TSS
seg_desc_t GDT[10];
tss_t      TSS;
// Création d'un descripteur dans la GDT 
#define gdt_flat_dsc(_dSc_,_pVl_,_tYp_)                                 \
   ({                                                                   \
      (_dSc_)->raw     = 0;                                             \
      (_dSc_)->limit_1 = 0xffff;                                        \
      (_dSc_)->limit_2 = 0xf;                                           \
      (_dSc_)->type    = _tYp_;                                         \
      (_dSc_)->dpl     = _pVl_;                                         \
      (_dSc_)->d       = 1;                                             \
      (_dSc_)->g       = 1;                                             \
      (_dSc_)->s       = 1;                                             \
      (_dSc_)->p       = 1;                                             \
})
// Création du descripteur tss dans la GDT
#define tss_dsc(_dSc_,_tSs_)                                            \
   ({                                                                   \
      raw32_t addr    = {.raw = _tSs_};                                 \
      (_dSc_)->raw    = sizeof(tss_t);                                  \
      (_dSc_)->base_1 = addr.wlow;                                      \
      (_dSc_)->base_2 = addr._whigh.blow;                               \
      (_dSc_)->base_3 = addr._whigh.bhigh;                              \
      (_dSc_)->type   = SEG_DESC_SYS_TSS_AVL_32;                        \
      (_dSc_)->p      = 1;                                              \
})
//Définition de nos descripteurs
#define c0_dsc(_d)  gdt_flat_dsc(_d,0,SEG_DESC_CODE_XR)
#define s0_t1_dsc(_d) gdt_flat_dsc(_d,0,SEG_DESC_DATA_RW)
#define s0_t2_dsc(_d) gdt_flat_dsc(_d,0,SEG_DESC_DATA_RW)
#define c3_t1_dsc(_d)  gdt_flat_dsc(_d,3,SEG_DESC_CODE_XR)
#define c3_t2_dsc(_d)  gdt_flat_dsc(_d,3,SEG_DESC_CODE_XR)
#define d3_t1_dsc(_d)  gdt_flat_dsc(_d,3,SEG_DESC_DATA_RW)
#define d3_t2_dsc(_d)  gdt_flat_dsc(_d,3,SEG_DESC_DATA_RW)
#define d3_dsc(_d)  gdt_flat_dsc(_d,3,SEG_DESC_DATA_RW)

//Initialisation gdt
void init_gdt() {
   gdt_reg_t gdtr;

   GDT[0].raw = 0ULL;

   c0_dsc( &GDT[c0_idx] );
   s0_t1_dsc( &GDT[s0_t1_idx] );
   s0_t2_dsc( &GDT[s0_t2_idx] );
   c3_t1_dsc( &GDT[c3_t1_idx] );
   c3_t2_dsc( &GDT[c3_t2_idx] );
   d3_t1_dsc( &GDT[d3_t1_idx] );
   d3_t2_dsc( &GDT[d3_t2_idx] );
   d3_dsc( &GDT[d3_idx] );
   
   gdtr.desc  = GDT;
   gdtr.limit = sizeof(GDT) - 1;
   set_gdtr(gdtr);

   set_cs(c0_sel);
   set_ss(s0_t1_sel);

   debug("\n GDT: \n");
   print_gdt_content(gdtr);
}



//------------------------------------------------------------- PAGINATION -------------------------------------------------------------

#define ADR_PILE_KNL_T1 0x301000
#define ADR_PILE_USR_T1 0x301000
#define ADR_PILE_KNL_T2 0x301000
#define ADR_PILE_USR_T2 0x301000
#define ADR_MEM_PARTAGE 0x301000
#define ADR_PILE_KNL 0x301000

void init_pagin(){
   //Pagination Tâche 1
   //Initialisation
   pde32_t * PGD_t1 = (pde32_t * )0x300000;
   pte32_t * PDT_t1_1 = (pte32_t * )0x301000;
   pte32_t * PDT_t1_2 = (pte32_t * )0x302000;
   memset((void*)PGD_t1, 0, 4096);
   memset(PDT_t1_1, 0, 4096);
   memset(PDT_t1_2, 0, 4096);
   //Première entrée PGD_t1 => PDT_t1_1 => Noyau
   pg_set_entry(&PGD_t1[0], PG_KRN|PG_RW, page_get_nr(PDT_t1_1));
   pg_set_entry(&PDT_t1_1[0], PG_KRN|PG_RW, page_get_nr(ADR_PILE_KNL_T1)); //Pile noyau
   //Deuxième entrée PGD_t1 => PDT_t1_2 => User
   pg_set_entry(&PGD_t1[1], PG_USR|PG_RW, page_get_nr(PDT_t1_2));
   pg_set_entry(&PDT_t1_2[0], PG_USR|PG_RW, page_get_nr(ADR_PILE_USR_T1)); //Pile user
   pg_set_entry(&PDT_t1_2[1], PG_USR|PG_RW, page_get_nr(ADR_MEM_PARTAGE)); //Mémoire partagée


   //Pagination Tâche 2
   //Initialisation
   pde32_t * PGD_t2 = (pde32_t * )0x400000;
   pte32_t * PDT_t2_1 = (pte32_t * )0x401000;
   pte32_t * PDT_t2_2 = (pte32_t * )0x402000;
   memset((void*)PGD_t2, 0, 4096);
   memset(PDT_t2_1, 0, 4096);
   memset(PDT_t2_2, 0, 4096);
   //Première entrée PGD_t2 => PDT_t2_1 => Noyau
   pg_set_entry(&PGD_t2[0], PG_KRN|PG_RW, page_get_nr(PDT_t2_1));
   pg_set_entry(&PDT_t2_1[0], PG_KRN|PG_RW, page_get_nr(ADR_PILE_KNL_T2)); //Pile noyau
   //Deuxième entrée PGD_t2 => PDT_t2_2 => User
   pg_set_entry(&PGD_t2[1], PG_USR|PG_RW, page_get_nr(PDT_t2_2));
   pg_set_entry(&PDT_t2_2[0], PG_USR|PG_RW, page_get_nr(ADR_PILE_USR_T2)); //Pile user
   pg_set_entry(&PDT_t2_2[2], PG_USR|PG_RW, page_get_nr(ADR_MEM_PARTAGE)); //Mémoire partagée (entrée 2 pour adr virtuelle différente de t1)


   //Pagination Noyau
   //Initialisation
   pde32_t * PGD_kn = (pde32_t * )0x500000;
   pte32_t * PDT_kn = (pte32_t * )0x501000;
   memset((void*)PGD_kn, 0, 4096);
   memset(PDT_kn, 0, 4096);
   //Première entrée PGD_kn => PDT_kn => Noyau
   pg_set_entry(&PGD_kn[0], PG_KRN|PG_RW, page_get_nr(PDT_kn));
   pg_set_entry(&PDT_kn[0], PG_KRN|PG_RW, page_get_nr(ADR_PILE_KNL));
}





//------------------------------------------------------------- INTERRUPTIONS -------------------------------------------------------------



//Initialisation IDT
idt_reg_t init_idt(){
	idt_reg_t ptr_idtr;
   get_idtr(ptr_idtr);
	debug("IDT adress: 0x%x\n", (unsigned int)(ptr_idtr.addr));
   return ptr_idtr;
}

void pic_enable_irq(uint8_t irq)
{
    pic_ocw1_t ocw1;
    uint16_t port;

    if (irq < 8) {
        port = PIC_IMR(PIC1);
    } else {
        port = PIC_IMR(PIC2);
        irq -= 8;
    }

    ocw1.raw = in(port);
    ocw1.raw &= ~(1 << irq);   // démasque IRQ
    out(ocw1.raw, port);
}


//Structure task pour identifier les piles
typedef struct task{
   uint32_t esp;
} task_t ;
static task_t task1,task2;
static task_t *current_task;


//Fonction du switch de pile post irq0
uint32_t int32_handler(uint32_t esp) {
   debug("j'entre dans le int32 handler \n");

   current_task->esp = esp;

   if (current_task == &task1){
      current_task = &task2;
   }
   else current_task = &task1;

   debug("je sors du int32 handler \n");

   pic_eoi(PIC1); //Permet d'envoyer un End Of Interrupt au CPU pour que irq0 puisse se redéclencher

   return current_task->esp;
}

//Import du handler int32 en assembleur
extern void int32_stub(void);

//Handler int80
void syscall_isr() {
   asm volatile (
      "leave ; pusha        \n"
      "mov %esp, %eax      \n"
      "call syscall_handler \n"
      "popa ; iret"
      );
}

void __regparm__(1) syscall_handler(int_ctx_t *ctx) {
   debug("SYSCALL eax = %p\n", (void *) ctx->gpr.eax.raw);
   debug("print syscall: %s", (char *)ctx->gpr.esi.raw);
}


//Compteur
void sys_counter(uint32_t *counter);

//Tâches 1 et 2
void user1(){
   while(1){
      debug("... I am User 1 ...\n");
      //TODO : Ecrire compteur
   }
}
void user2(){
   while(1){
      debug("... I am User 2 ...\n");
      //TODO : Lire compteur
      //asm volatile ("int $80");
   }
}




//------------------------------------------------------------- MAIN -------------------------------------------------------------




void tp() {
	
	//Affichage de la mémoire 
	print_mem();

   //Initialisation de la GDT en mode flat + TSS
   init_gdt();
  
   //Initialisation IDT
	idt_reg_t ptr_idtr = init_idt();

   //Affichage cr3
	uint32_t cr3 = get_cr3();
	debug("cr3 : %x\n", cr3);

   //Initialisation pagination
   init_pagin();

   // Activation pagination
   //set_cr3(PGD);
	//set_cr0(CR0_PG);

   //Changer addresse du handler de int32
	int_desc_t * desc32 = &(ptr_idtr.desc[32]);
	desc32->offset_1 = (uint32_t)int32_stub;
	desc32->offset_2 = ((uint32_t)int32_stub)>>16;
   desc32->dpl = 0;
   desc32->p=1;
   desc32->selector = c0_sel;
  
   //Activation de irq0
   pic_enable_irq(PIC_TIMER_IRQ); // IRQ0

   // Changer addresse du handler de int80
	int_desc_t * desc = &(ptr_idtr.desc[80]);
	desc->offset_1 = (uint32_t)syscall_isr;
	desc->offset_2 = ((uint32_t)syscall_isr)>>16;
   desc->dpl = 3;



}

/*
TODO
- Bien construire les tâches et leur piles avec le contexte empilé pour préparer le 1er dépilement post irq0
*/


 /*
 EXPLICATIONS INTERRUPTION irq0
   Quand irq0 se déclenche: 
   - cpu sauvegarde eip, cs, eflags (ss/esp si changement de ring)
   - saute dans mon stub assembleur
   => la pile est la pile de la tâche interrompue, le contexte est sur cette pile

   quand j'appelle mon stub asm, je vais sauvegarder le contexte de ma tâche interrompue
   puis je vais appeler mon échangeur de tâches int_handler
   qui va indiquer le esp de l'autre tâche
   donc ensuite je reprend le cours de mon stub et je vais effectuer le switch d'esp
   puis dépiler mes éléments de contexte

   à la définition de mes tâches, il faut construire leur piles avec le bon contexte empilé 
   sinon il y aura forcément un problème au 1er switch
*/