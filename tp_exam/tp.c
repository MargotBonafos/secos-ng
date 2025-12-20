/* GPLv2 (c) Airbus */
#include <debug.h>
#include <segmem.h>
#include <grub_mbi.h>
#include <info.h>

extern info_t   *info;

extern uint32_t __kernel_start__, __kernel_end__;
extern uint32_t __user_start__, __user_end__;
extern uint32_t __user1_stack_base__, __user1_stack_end__;
extern uint32_t __user2_stack_base__, __user2_stack_end__;
extern uint32_t __shared_base__, __shared_end__;

#define __user__ __attribute__((section(".user")))

#define __user_data__ __attribute__((section(".user_data"),aligned(4096))) // Pour les piles utilisateurs

#define USER_STACK_SIZE 0x1000 // Piles utilisateurs de 4KB

// Tache user 1
void __user__ user1() {
    while(1){
        printf("Je suis user 1\n");
    }	
	//asm volatile ("mov %eax, %cr0");
}

// Tache user 2
void __user__ user2() {
    while(1){
        printf("Je suis user 2\n");
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

	//Creation de notre GDT//
	
	seg_desc_t my_gdt[9];

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

    
}
