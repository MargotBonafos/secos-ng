/* GPLv2 (c) Airbus */
#include <debug.h>
#include <segmem.h>
#include <grub_mbi.h>
#include <info.h>
#include <cr.h>
#include <pagemem.h>
#include <intr.h>
#include <pic.h>
#include <io.h>

#include <configuration_files/pagination/paging_setup.h>
#include <configuration_files/segmentation/segmentation_setup.h>
#include <configuration_files/tss/tss_setup.h>
#include <task.h>

extern info_t   *info;

// Les codes users et noyau
extern uint32_t __kernel_start__, __kernel_end__;
extern uint32_t __user_start__, __user_end__;

// Les piles ring3 user 1 et user 2
extern uint32_t __user1_stack_base__, __user1_stack_end__;
extern uint32_t __user2_stack_base__, __user2_stack_end__;

// La zone partagee
extern uint32_t __shared_base__, __shared_end__;

// Les piles ring0 user 1, user 2 et noyau
extern uint32_t __kernel_stack_user1_base__, __kernel_stack_user1_end__;
extern uint32_t __kernel_stack_user2_base__, __kernel_stack_user2_end__;
extern uint32_t __kernel_stack_base__, __kernel_stack_end__;


#define __user__ __attribute__((section(".user")))

#define __user_data__ __attribute__((section(".user_data"),aligned(4096))) // Pour les piles utilisateurs

#define USER_STACK_SIZE 0x1000 // Piles utilisateurs de 4KB

task_t task_user1;
task_t task_user2;
task_t *current_task = 0;

// Tache user 1
void __user__ user1() {

    printf(" Bienvenue user 1\n");

    while(1){
        printf("Je suis user 1\n");
    }	
	//asm volatile ("mov %eax, %cr0");
}

// Tache user 2
void __user__ user2() {

    printf(" Bienvenue user 2\n");

    while(1){
        printf("Je suis user 2\n");
    }	
	//asm volatile ("mov %eax, %cr0");
}

void tp() {

    /* ---------------  Configuration de la memoire physique ------------------- */

    // cf linker.lds pour comprendre l'agencement de la memoire

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

    // Declaration de la pile noyau user 1
    __attribute__((section(".kernel_stack_user1"), aligned(4096)))
    static uint8_t kernel_stack_user1[4096];
    debug("kernel_stack_user1 base=%p size=0x%x top(ESP init)=%p\n", kernel_stack_user1, (unsigned)USER_STACK_SIZE, kernel_stack_user1 + USER_STACK_SIZE);

    // Declaration de la pile noyau user 2
    __attribute__((section(".kernel_stack_user2"), aligned(4096)))
    static uint8_t kernel_stack_user2[4096];
    debug("kernel_stack_user2 base=%p size=0x%x top(ESP init)=%p\n", kernel_stack_user2, (unsigned)USER_STACK_SIZE, kernel_stack_user2 + USER_STACK_SIZE);

    // Declaration de la pile noyau
    __attribute__((section(".kernel_stack"), aligned(4096)))
    static uint8_t kernel_stack[4096];
    debug("kernel_stack base=%p size=0x%x top(ESP init)=%p\n", kernel_stack, (unsigned)USER_STACK_SIZE, kernel_stack + USER_STACK_SIZE);


    /* ---------------  Configuration de segmentation ------------------- */

    // Utilisation de segmentation_setup.h

    // Creation de la GDT et mise a jour de GDTR et des selecteurs de segments avec code et pile noyau
	segmentation_setup_gdt();

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


    /* ---------------  Configuration de la TSS ------------------- */
    
    // Utilisation de tss_setup.h
    tss_setup(); // Mise de esp ring0 et ss ring0 dans TSS et mise a jour de TR

    
   /* ---------------  Configuration de la pagination ------------------- */

   /* -- Configuration des PGD et PTB (Utilisation de paging_setup.h) -- */
   
   paging_setup_kernel(); // met la PGD de kernel dans CR3
   
   paging_setup_user1();
   paging_setup_user2();

    // Activer Pagination
    uint32_t cr0 = get_cr0();
    set_cr0(cr0 | (1u << 31)); 

    
    /* ---------------  Configuration des interruptions ------------------- */


    /* ---------------  Initialisation en mode user1 ------------------- */

    current_task = &task_user1;

    // changer la pile noyau de la TSS pour la pile noyau user 1
    TSS.s0.esp = __kernel_stack_user1_end__; // adresse 0x00c0 1000
    TSS.s0.ss  = gdt_krn_seg_sel(2);

}
