/* GPLv2 (c) Airbus */
#include <debug.h>
#include <info.h>
#include <segmem.h>
#include <string.h>
#include <pagemem.h>
#include <cr.h>
#include <intr.h>
#include <pic.h>


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

idt_reg_t init_idt(){
	idt_reg_t ptr_idtr;
   get_idtr(ptr_idtr);
	debug("IDT adress: 0x%x\n", (unsigned int)(ptr_idtr.addr));
   return ptr_idtr;
}

typedef struct task{
   uint32_t esp;
} task_t

task_t task1,task2;

uint32_t int32_handler(uint32_t esp) {
   debug("j'entre dans le int32 handler \n");
   //ici je fais mon switch de esp
   uint32_t esp_courant;
   if (esp == &task1->esp){
      esp_courant = &task2->esp;
   }
   else esp_courant = &task1->esp;
   debug("je sors du int32 handler \n");
   return esp_courant;
}




void int32_trigger(){
   asm volatile ("int $32");
}
void tp() {
	
	//Affichage de la mémoire 
	print_mem();

   //Initialisation de la GDT en mode flat + TSS
   init_gdt();
  
   //Initialisation IDT
	idt_reg_t ptr_idtr = init_idt();

   //Changer addresse du handler de int32
	int_desc_t * desc = &(ptr_idtr.desc[32]);
	desc->offset_1 = (uint32_t)int32_handler;
	desc->offset_2 = ((uint32_t)int32_handler)>>16;
   //peut-être ajouter aussi le dpl, le selecteur de sgment...?
   //chat me dit de ne pas faire pointer vers mon int_handler mais vers un int_stub
   //le stub contiendrais le  pushad, call int_handler, popad, cli...
   /*
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

   pic_enable_irq(PIC_TIMER_IRQ); // IRQ0

   //Déclenchement de int 32 pour tester
   //int32_trigger();
}

/*
TODO
- Trouver comment doit être écrit le stub asm (fonction, fichier...) 
- Trouver comment l'appeler (le référencer dans l'idt en fait je crois)
- Ecrire le stub asm
- Est-ce qu'on peut pas tout mettre dans mon int_handler?
*/