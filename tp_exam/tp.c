/* GPLv2 (c) Airbus */
#include <debug.h>
#include <segmem.h>
#include <grub_mbi.h>
#include <info.h>
#include <cr.h>
#include <pagemem.h>

extern info_t   *info;

// Les codes users et noyeau
extern uint32_t __kernel_start__, __kernel_end__;
extern uint32_t __user_start__, __user_end__;

// Les piles ring3 user 1 et user 2
extern uint32_t __user1_stack_base__, __user1_stack_end__;
extern uint32_t __user2_stack_base__, __user2_stack_end__;

// La zone partagee
extern uint32_t __shared_base__, __shared_end__;

// Les piles ring0 user 1, user 2 et noyeau
extern uint32_t __kernel_stack_user1_base__, __kernel_stack_user1_end__;
extern uint32_t __kernel_stack_user2_base__, __kernel_stack_user2_end__;
extern uint32_t __kernel_stack_base__, __kernel_stack_end__;


#define __user__ __attribute__((section(".user")))

#define __user_data__ __attribute__((section(".user_data"),aligned(4096))) // Pour les piles utilisateurs

#define USER_STACK_SIZE 0x1000 // Piles utilisateurs de 4KB

tss_t TSS;

typedef struct {
  uint32_t kernel_base;
  uint32_t kernel_esp;
  uint32_t cr3;
} task_t;

task_t task_user1,task_user2;

// Tache user 1
void __user__ user1() {

    printf("Je suis user 1\n");

    while(1){
        
    }	
	//asm volatile ("mov %eax, %cr0");
}

// Tache user 2
void __user__ user2() {

    printf("Je suis user 2\n");

    while(1){
        
    }
	//asm volatile ("mov %eax, %cr0");
}


// fonction qui affiche la GDT 
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


// Fonction utile pour voir l'etat de la memoire
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

void tp() {

	/*****  Creation de la GDT *****/ 
	
	seg_desc_t my_gdt[11];

    my_gdt[0].raw = 0ULL;

    //GDT[1] : Code ring 0 - Noyeau 
    my_gdt[1].limit_1 = 0xFFFF;
    my_gdt[1].base_1 = 0x0000;
    my_gdt[1].base_2 = 0x00;
    my_gdt[1].type = 11;
    my_gdt[1].s = 1;
    my_gdt[1].dpl = 0;
    my_gdt[1].p = 1;
    my_gdt[1].limit_2 = 0xF;
    my_gdt[1].avl = 1;
    my_gdt[1].l = 0;
    my_gdt[1].d = 1;
    my_gdt[1].g = 1;
    my_gdt[1].base_3 = 0x00;

    // Pile Noyeau ring 0 - User 1
    my_gdt[2].limit_1 = 0xFFFF;
    my_gdt[2].base_1 = 0x0000;
    my_gdt[2].base_2 = 0x00;
    my_gdt[2].type = 3;
    my_gdt[2].s = 1;
    my_gdt[2].dpl = 0;
    my_gdt[2].p = 1;
    my_gdt[2].limit_2 = 0xF;
    my_gdt[2].avl = 1;
    my_gdt[2].l = 0;
    my_gdt[2].d = 1;
    my_gdt[2].g = 1;
    my_gdt[2].base_3 = 0x00;

    // Pile Noyeau ring 0 - User 2
    my_gdt[3].limit_1 = 0xFFFF;
    my_gdt[3].base_1 = 0x0000;
    my_gdt[3].base_2 = 0x00;
    my_gdt[3].type = 3;
    my_gdt[3].s = 1;
    my_gdt[3].dpl = 0;
    my_gdt[3].p = 1;
    my_gdt[3].limit_2 = 0xF;
    my_gdt[3].avl = 1;
    my_gdt[3].l = 0;
    my_gdt[3].d = 1;
    my_gdt[3].g = 1;
    my_gdt[3].base_3 = 0x00;

    // Code ring 3 - User 1
    my_gdt[4].limit_1 = 0xFFFF;
    my_gdt[4].base_1 = 0x0000;
    my_gdt[4].base_2 = 0x00;
    my_gdt[4].type = 11;
    my_gdt[4].s = 1;
    my_gdt[4].dpl = 3;
    my_gdt[4].p = 1;
    my_gdt[4].limit_2 = 0xF;
    my_gdt[4].avl = 1;
    my_gdt[4].l = 0;
    my_gdt[4].d = 1;
    my_gdt[4].g = 1;
    my_gdt[4].base_3 = 0x00;

    // Pile ring 3 - User 1
    my_gdt[5].limit_1 = 0xFFFF;
    my_gdt[5].base_1 = 0x0000;
    my_gdt[5].base_2 = 0x00;
    my_gdt[5].type = 3;
    my_gdt[5].s = 1;
    my_gdt[5].dpl = 3;
    my_gdt[5].p = 1;
    my_gdt[5].limit_2 = 0xF;
    my_gdt[5].avl = 1;
    my_gdt[5].l = 0;
    my_gdt[5].d = 1;
    my_gdt[5].g = 1;
    my_gdt[5].base_3 = 0x00;

    // Code ring 3 - User 2
    my_gdt[6].limit_1 = 0xFFFF;
    my_gdt[6].base_1 = 0x0000;
    my_gdt[6].base_2 = 0x00;
    my_gdt[6].type = 11;
    my_gdt[6].s = 1;
    my_gdt[6].dpl = 3;
    my_gdt[6].p = 1;
    my_gdt[6].limit_2 = 0xF;
    my_gdt[6].avl = 1;
    my_gdt[6].l = 0;
    my_gdt[6].d = 1;
    my_gdt[6].g = 1;
    my_gdt[6].base_3 = 0x00;

    // Pile ring 3 - User 2
    my_gdt[7].limit_1 = 0xFFFF;
    my_gdt[7].base_1 = 0x0000;
    my_gdt[7].base_2 = 0x00;
    my_gdt[7].type = 3;
    my_gdt[7].s = 1;
    my_gdt[7].dpl = 3;
    my_gdt[7].p = 1;
    my_gdt[7].limit_2 = 0xF;
    my_gdt[7].avl = 1;
    my_gdt[7].l = 0;
    my_gdt[7].d = 1;
    my_gdt[7].g = 1;
    my_gdt[7].base_3 = 0x00;

    // Data ring 3 - Partage User 1 & User 2
    my_gdt[8].limit_1 = 0xFFFF;
    my_gdt[8].base_1 = 0x0000;
    my_gdt[8].base_2 = 0x00;
    my_gdt[8].type = 3;
    my_gdt[8].s = 1;
    my_gdt[8].dpl = 3;
    my_gdt[8].p = 1;
    my_gdt[8].limit_2 = 0xF;
    my_gdt[8].avl = 1;
    my_gdt[8].l = 0;
    my_gdt[8].d = 1;
    my_gdt[8].g = 1;
    my_gdt[8].base_3 = 0x00;

    // Pile noyeau ring 0
    my_gdt[9].limit_1 = 0xFFFF;
    my_gdt[9].base_1 = 0x0000;
    my_gdt[9].base_2 = 0x00;
    my_gdt[9].type = 3;
    my_gdt[9].s = 1;
    my_gdt[9].dpl = 0;
    my_gdt[9].p = 1;
    my_gdt[9].limit_2 = 0xF;
    my_gdt[9].avl = 1;
    my_gdt[9].l = 0;
    my_gdt[9].d = 1;
    my_gdt[9].g = 1;
    my_gdt[9].base_3 = 0x00;

    // Mise a jour de la GDTR
    gdt_reg_t gdtr_of_my_gdt;
    gdtr_of_my_gdt.addr = (long unsigned int) my_gdt;
    gdtr_of_my_gdt.limit = sizeof(my_gdt) - 1;
    
    set_gdtr(gdtr_of_my_gdt); // Update du registre GDTR
    
    //Mettre a jour les selecteurs de segments
    set_cs(gdt_krn_seg_sel(1));
    set_ds(gdt_krn_seg_sel(2));

    // Verification de l'utilisation de my_gdt
    gdt_reg_t actual_gdtr;
    get_gdtr(actual_gdtr);

    // Affichage des valeurs gdtr_value (et donc implicitement des valeurs du registre GDTR)
    printf("taille de la GDT : %hu \n", actual_gdtr.limit);
    printf("addresse de la GDT : %lu  \n", actual_gdtr.addr);

    // Affichage du contenu de la GDT
    print_gdt_content(actual_gdtr);

    // Affichage des selecteurs de segments
    printf("DS : %hu \n", get_ds());
    printf("CS : %hu \n", get_seg_sel(cs));

    printf("SS : %hu \n", get_ss());
    printf("ES : %hu \n", get_es());
    printf("FS : %hu \n", get_fs());
    printf("GS : %hu \n", get_gs());

    /* --- Voir la plage d'adresse utilisee par le code noyeau pour l'identity mapping --- */
    debug("kernel mem [%p - %p]\n", &__kernel_start__, &__kernel_end__);

   // On obtient que le code noyeau est dans la page 0x0000 0000 - 0x0040 0000 -> Donc my_pgd[0] -> dpl = 0

   // Recuperation de la page de 4MB des codes users
   debug(".user [%p - %p]\n", &__user_start__, &__user_end__);

   // Verification que les piles utilisateurs ont ete mises aux bons endroits

   //debug("page pour pile user 1  [%p - %p]\n", &__user1_stack_base__, &__user1_stack_end__);

   //debug("page pour pile user 2  [%p - %p]\n", &__user2_stack_base__, &__user2_stack_end__);

   //debug("page pour donnees partagees  [%p - %p]\n", &__shared_base__, &__shared_end__);

   // Declaration de la pile user 1
    __attribute__((section(".user1_stack"), aligned(16)))
    static uint8_t user1_stack[USER_STACK_SIZE];
    debug("user1_stack base=%p size=0x%x top(ESP init)=%p\n", user1_stack, (unsigned)USER_STACK_SIZE, user1_stack + USER_STACK_SIZE);

    // Declaration de la pile user 2
    __attribute__((section(".user2_stack"), aligned(16)))
    static uint8_t user2_stack[USER_STACK_SIZE];
    debug("user2_stack base=%p size=0x%x top(ESP init)=%p\n", user2_stack, (unsigned)USER_STACK_SIZE, user2_stack + USER_STACK_SIZE);

    // Declaration de la zone partagee
    __attribute__((section(".shared"), aligned(4096)))
    static uint8_t shared_page[4096];
    debug("shared_page base=%p size=0x%x top(ESP init)=%p\n", shared_page, (unsigned)USER_STACK_SIZE, shared_page + USER_STACK_SIZE);

    // Declaration de la pile noyeau user 1
    __attribute__((section(".kernel_stack_user1"), aligned(4096)))
    static uint8_t kernel_stack_user1[4096];
    debug("kernel_stack_user1 base=%p size=0x%x top(ESP init)=%p\n", kernel_stack_user1, (unsigned)USER_STACK_SIZE, kernel_stack_user1 + USER_STACK_SIZE);

    // Declaration de la pile noyeau user 2
    __attribute__((section(".kernel_stack_user2"), aligned(4096)))
    static uint8_t kernel_stack_user2[4096];
    debug("kernel_stack_user2 base=%p size=0x%x top(ESP init)=%p\n", kernel_stack_user2, (unsigned)USER_STACK_SIZE, kernel_stack_user2 + USER_STACK_SIZE);

    /*****  Creation de la TSS *****/
    memset(&TSS, 0, sizeof(TSS));

   // Si le CPU rentres en ring 0, il utilisera cette pile -> Changera a chaque changement de tache
   TSS.s0.esp = get_ebp();            // haut de la pile kernel (ring 0)
   TSS.s0.ss  = gdt_krn_seg_sel(9);   // segment data ring 0

   // Construction du descripteur TSS dans la GDT à l'indice 10
   {
    uint32_t base  = (uint32_t)&TSS;
    uint32_t limit = sizeof(TSS) - 1;

    my_gdt[10].limit_1 = limit & 0xFFFF;
    my_gdt[10].base_1  = base & 0xFFFF;
    my_gdt[10].base_2  = (base >> 16) & 0xFF;
    my_gdt[10].type    = 0x9;
    my_gdt[10].s       = 0;
    my_gdt[10].dpl     = 0;
    my_gdt[10].p       = 1;
    my_gdt[10].limit_2 = (limit >> 16) & 0xF;
    my_gdt[10].avl     = 0;
    my_gdt[10].l       = 0;
    my_gdt[10].d       = 0;
    my_gdt[10].g       = 0;
    my_gdt[10].base_3  = (base >> 24) & 0xFF;
   }

   set_tr(gdt_krn_seg_sel(10)); // On met l'entree TSS de la GDT dans le registre TR


   /*********  Configuration de la pagination  *********/

    /* ---- PGD de user 1 ----*/
    pde32_t* pgd_user1 = (pde32_t*) 0x610000;
	
    task_user1.cr3 = (uint32_t) pgd_user1;
   
    // Creation d'une PTB
	pte32_t* ptb_0_user1 = (pte32_t*) 0x611000;

    // Mise a 0 de toutes les entrees de la PGD
	memset((void*)pgd_user1, 0, PAGE_SIZE);

    // Configuration de l'identity mapping pour la pile user 1 ring 3 (1 page 4KB)
    

    // Configuration de l'identity mapping pour codes users



    // Configuration du mapping pour la zone partagee

    // Creation d'une PTB
	pte32_t* ptb_704_user1 = (pte32_t*) 0x61?000;

    pgd_user1[704].p = 1;
	pgd_user1[704].rw = 1;
	pgd_user1[704].lvl = 3; // table de page accessible en ring 3
	pgd_user1[704].pwt = 0;
	pgd_user1[704].pcd = 0;
	pgd_user1[704].acc = 0;
	pgd_user1[704].mbz = 0;
	pgd_user1[704].avl = 0;
	pgd_user1[704].addr = page_get_nr(ptb_704_user1);

    ptb_704_user1[0].p = 1;
	ptb_704_user1[0].rw = 1;
	ptb_704_user1[0].lvl = 3; // Page accessible en ring 3
	ptb_704_user1[0].pwt = 0;
	ptb_704_user1[0].pcd = 0;
	ptb_704_user1[0].acc = 0;
	ptb_704_user1[0].d = 0;
	ptb_704_user1[0].pat = 0;
	ptb_704_user1[0].g = 0; // Page non globale (elle disparait au flush de la TLB)
	ptb_704_user1[0].avl = 0;
	ptb_704_user1[0].addr = page_get_nr(0x802000); // Adresse de la page physique 


    /* ---- PGD de user 2 ----*/
    pde32_t* pgd_user2 = (pde32_t*) 0x620000;
	
    task_user1.cr3 = (uint32_t) pgd_user2;
   
    // Creation d'une PTB
	pte32_t* ptb_0_user2 = (pte32_t*) 0x621000;

    // Mise a 0 de toutes les entrees de la PGD
	memset((void*)pgd_user2, 0, PAGE_SIZE);

    // Configuration de l'identity mapping pour la pile user 1 ring 3

    // Configuration de l'identity mapping pour codes users
   


    // Configuration du mapping pour la zone partagee

    // Creation d'une PTB
	pte32_t* ptb_2_user2 = (pte32_t*) 0x622000;

    pgd_user2[2].p = 1;
	pgd_user2[2].rw = 1;
	pgd_user2[2].lvl = 3; // table de page accessible en ring 3
	pgd_user2[2].pwt = 0;
	pgd_user2[2].pcd = 0;
	pgd_user2[2].acc = 0;
	pgd_user2[2].mbz = 0;
	pgd_user2[2].avl = 0;
	pgd_user2[2].addr = page_get_nr(ptb_2_user2);

    ptb_2_user2[724].p = 1;
	ptb_2_user2[724].rw = 1;
	ptb_2_user2[724].lvl = 3; // Page accessible en ring 3
	ptb_2_user2[724].pwt = 0;
	ptb_2_user2[724].pcd = 0;
	ptb_2_user2[724].acc = 0;
	ptb_2_user2[724].d = 0;
	ptb_2_user2[724].pat = 0;
	ptb_2_user2[724].g = 0; // Page non globale (elle disparait au flush de la TLB)
	ptb_2_user2[724].avl = 0;
	ptb_2_user2[724].addr = page_get_nr(0x802000); // Adresse de la page physique 


    /* ---- PGD du noyeau ----*/
    pde32_t* pgd_kernel = (pde32_t*) 0x600000;
	
    task_user1.cr3 = (uint32_t) pgd_kernel;
   
    // Creation d'une PTB
	pte32_t* ptb_0_kernel = (pte32_t*) 0x601000;

    // Mise a 0 de toutes les entrees de la PGD
	memset((void*)pgd_kernel, 0, PAGE_SIZE);

    // Configuration de l'identity mapping pour la pile user 1 ring 3

    // Configuration de l'identity mapping pour codes users

    // Configuration du mapping pour la zone partagee

    // Creation d'une PTB
	pte32_t* ptb_2_kernel = (pte32_t*) 0x601000;



    // Activer Pagination
    set_cr3((uint32_t)pgd_noyeau);
    uint32_t cr0 = get_cr0();
    set_cr0(cr0 | (1u << 31)); // PG=1

    // Autre / suite

    // changer la pile noyeau de la TSS pour la pile noyeau user 1
    TSS.s0.esp = __kernel_stack_user1_end__; // adresse 0x00c0 1000
    TSS.s0.ss  = gdt_krn_seg_sel(2);

}
