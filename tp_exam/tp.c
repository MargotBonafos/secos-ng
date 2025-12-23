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
#include <configuration_files/interruption/interruption_setup.h>
#include <configuration_files/syscall/syscall.h>

#include <task.h>

#include <autre/affichage_en_tete.h>

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

    volatile uint32_t *counter = (volatile uint32_t*)VADDR_COUNTER_USER1; // expose par syscall.h

    while (1){

        // Petit delai afin de voir l'incrementation de 1 en 1 dans les affichages de user 2
        for (volatile int i=0; i<4000000; i++){
            asm volatile("nop");
        }

        (*counter)++;
    }
}

// Tache user 2
void __user__ user2() {

    volatile uint32_t *counter = (volatile uint32_t*)VADDR_COUNTER_USER2; // expose par syscall.h

    while (1){
    
        // Appel systeme pour l'affichage de la valeur du compteur par ring 0
        asm volatile(
        "movl %0, %%eax \n\t"
        "int $0x80      \n\t"
        :
        : "r"(counter)
        : "eax", "memory"
        );
    }
}

void tp() {

    /* ---------------  Affichage de l'en tete du projet ------------------- */

    afficher_en_tete();

    /* ---------------  Configuration de la memoire physique ------------------- */

    // cf linker.lds pour comprendre l'agencement de la memoire

    debug("\n\n");
    debug("--- Affichage de l'organisation memoire physique (RAM) ---\n\n");
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

    debug("\n\n");

    /* ---------------  Configuration de segmentation ------------------- */

    // Utilisation de segmentation_setup.h

    // Creation de la GDT et mise a jour de GDTR et des selecteurs de segments avec code et pile noyau
	segmentation_setup_gdt();

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
    
    // Mise a jour des CR3 des struct des tasks
    task_user1.cr3 = get_user1_pgd_addr();
    task_user2.cr3 = get_user2_pgd_addr();

    // debug pour verif
    /*
    debug("pgd_user1=%d pgd_user2=%d task1.cr3=0x%x task2.cr3=0x%x\n",
      get_user1_pgd_addr(), get_user2_pgd_addr(), task_user1.cr3, task_user2.cr3);
    */

    /* ---------------  Configuration des interruptions ------------------- */
    
    // Cf interruption_setup.c pour voir toutes les actions de configuration
    interruption_setup(1); // Fixe a 50 interruptions irq0 / seconde


    /* ---------------  Preparer la frame d'interruption de user 2 ------------------- */
    /* Necessaire pour le premier switch de contexte de intr_hdlr */
    
    /*** Creation des selecteurs de segments ***/

    // --- Creation du SS ring3 USER 2 ---
    seg_sel_t ss_ring3_user2;

    //segment dans la GDT
    ss_ring3_user2.index = 7;
    ss_ring3_user2.ti = 0;
    ss_ring3_user2.rpl = 3;

    uint16_t ss_selecteur_ring3_user2 = (ss_ring3_user2.index << 3) | (ss_ring3_user2.ti << 2) | (ss_ring3_user2.rpl);

    // --- Creation de ESP ---
    uint32_t esp_ring3_user2 = (uint32_t)user2_stack + sizeof(user2_stack);

    // --- Creation du CS ring3 ---
    seg_sel_t cs_ring3_user2;

    cs_ring3_user2.index = 6; //segment dans la GDT
    cs_ring3_user2.ti = 0;
    cs_ring3_user2.rpl = 3;

    uint16_t cs_selecteur_ring3_user2 = (cs_ring3_user2.index << 3) | (cs_ring3_user2.ti << 2) | (cs_ring3_user2.rpl);


    /*** Creation d'un int_ctx_t ***/

    // On recupere le esp de la pile noyau user 2
    uint32_t *sp = (uint32_t*)&__kernel_stack_user2_end__;

    // On Reserver la place de int_ctx_t sur la pile noyau
    sp = (uint32_t *)((uint8_t*)sp - sizeof(int_ctx_t));

    int_ctx_t *frame = (int_ctx_t*)sp;

    // Push des registres generaux (pour que le popa fonctionne)
    frame->gpr.eax.raw = 0;
    frame->gpr.ecx.raw = 0;
    frame->gpr.edx.raw = 0;
    frame->gpr.ebx.raw = 0;
    frame->gpr.esp.raw = 0;
    frame->gpr.ebp.raw = 0;
    frame->gpr.esi.raw = 0;
    frame->gpr.edi.raw = 0;

    // Champs CPU [ ajoute par le stub d'habitude (nr, err)]
    frame->nr.raw  = 32;      // comme si ça venait de IRQ0
    frame->err.raw = 0;

    frame->eip.raw = (uint32_t)user2;
    frame->cs.raw  = cs_selecteur_ring3_user2;
    frame->eflags.raw = EFLAGS_IF; // assure IF=1

    frame->esp.raw = esp_ring3_user2;
    frame->ss.raw  = ss_selecteur_ring3_user2;

    // Mise a jour de kernel_esp de la structure afin qu'il soit correctement 
    // Utilise dans intr_hdlr au moment du switch    
    task_user2.kernel_esp = (uint32_t)frame;


    /* ---------------  Initialisation en mode user1 ------------------- */

    current_task = &task_user1;

    /* Mise en place du contexte de user 1 sur la pile noyau pour qu'au moment du iret,
       on passe dans le code user1 */

    // --- Creation du SS ring3 User 1 ---
    seg_sel_t ss_ring3_user1;

    //segment dans la GDT
    ss_ring3_user1.index = 5;
    ss_ring3_user1.ti = 0;
    ss_ring3_user1.rpl = 3;

    uint16_t ss_selecteur_ring3_user1 = (ss_ring3_user1.index << 3) | (ss_ring3_user1.ti << 2) | (ss_ring3_user1.rpl);

    // --- Creation de ESP User 1 ---
    uint32_t esp_ring3_user1 = (uint32_t)user1_stack + sizeof(user1_stack);

    // --- Creation du CS ring3 User 1 ---
    seg_sel_t cs_ring3_user1;

    cs_ring3_user1.index = 4; //segment dans la GDT
    cs_ring3_user1.ti = 0;
    cs_ring3_user1.rpl = 3;

    uint16_t cs_selecteur_ring3_user1 = (cs_ring3_user1.index << 3) | (cs_ring3_user1.ti << 2) | (cs_ring3_user1.rpl);

    // --- Autres actions ---

    // changer la pile noyau de la TSS pour la pile noyau user 1
    TSS.s0.esp = (uint32_t)&__kernel_stack_user1_end__; // adresse 0x00c0 1000
    TSS.s0.ss  = gdt_krn_seg_sel(2);

    // Mise a jour de CR3
    set_cr3(task_user1.cr3);

    // Mise a jour de current task
    current_task = &task_user1;

    // Mise a 0 du compteur
    *(volatile uint32_t*)VADDR_COUNTER_USER1 = 0;

    // Affichage uniquement du compteur a partir d'ici
    debug("--- Affichage du compteur ---\n\n");

    // Mise de IF a 1 pour reactiver irq0 (equivalent a sti())
    uint32_t eflags;
    asm volatile("pushf; pop %0" : "=r"(eflags));
    eflags |= (1u << 9);   // IF = 1

    // --- Push du contexte ---

    asm volatile(
        "pushl %[ss]\n\t"
        "pushl %[uesp]\n\t"
        "pushl %[efl]\n\t"
        "pushl %[cs]\n\t"
        "pushl %[eip]\n\t"
        "iret\n\t"
        :
        : [ss]   "r"((uint32_t)ss_selecteur_ring3_user1),
        [uesp] "r"(esp_ring3_user1),
        [efl]  "r"(eflags),
        [cs]   "r"((uint32_t)cs_selecteur_ring3_user1),
        [eip]  "r"((uint32_t)user1)
        : "memory"
    );
    

}
