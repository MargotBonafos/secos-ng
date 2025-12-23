/* GPLv2 (c) Airbus */
#include <debug.h>
#include <cr.h>
#include <pagemem.h>
#include <string.h>
#include "paging_setup.h"

extern uint32_t __kernel_start__, __kernel_end__;
extern uint32_t __user_start__,   __user_end__;
extern uint32_t __user1_stack_base__, __user1_stack_end__;
extern uint32_t __shared_base__, __shared_end__;
extern uint32_t __kernel_stack_user1_base__, __kernel_stack_user1_end__;
extern uint32_t __kernel_stack_user2_base__, __kernel_stack_user2_end__;

// Choix : VA partagee
#define USER1_SHARED_VA   ((void*)0xb0001000u)   // PDE=704, PTE=1

// Adresses des tables
#define U1_PGD_PA   0x00610000u
#define U1_PTB0_PA  0x00611000u  // PDE 0 : 0..4MB
#define U1_PTB1_PA  0x00612000u  // PDE 1 : 4..8MB
#define U1_PTB2_PA  0x00613000u  // PDE 2 : 8..12MB (pile user1, shared phys)
#define U1_PTB3_PA  0x00614000u  // PDE 3 : 12..16MB (piles noyau à 0x00c00000)
#define U1_PTB704_PA 0x00615000u  // PDE 320 : shared VA

static inline void ptb_clear(pte32_t *ptb) {
    memset((void*)ptb, 0, PAGE_SIZE);
}
static inline void pgd_clear(pde32_t *pgd) {
    memset((void*)pgd, 0, PAGE_SIZE);
}

static inline void map_ptb_identity(pte32_t *ptb, uint32_t base_frame, uint32_t flags) {
    // remplit 1024 PTE => 4MB
    for (int i = 0; i < 1024; i++) {
        pg_set_entry(&ptb[i], flags, base_frame + (uint32_t)i);
    }
}

// mappe une page dans une PTB avec va -> pa
static inline void map_page_in_ptb(pte32_t *ptb, void *va, uint32_t pa, uint32_t flags) {
    int pti = pt32_get_idx(va);
    pg_set_entry(&ptb[pti], flags, page_get_nr(pa));
}

void paging_setup_user1(void)
{
    pde32_t *pgd_user1 = (pde32_t*)U1_PGD_PA;

    pte32_t *ptb0_user1  = (pte32_t*)U1_PTB0_PA;
    pte32_t *ptb1_user1  = (pte32_t*)U1_PTB1_PA;
    pte32_t *ptb2_user1  = (pte32_t*)U1_PTB2_PA;
    pte32_t *ptb3_user1  = (pte32_t*)U1_PTB3_PA;
    pte32_t *ptb704_user1 = (pte32_t*)U1_PTB704_PA;

    // on remplit toutes nos tables a 0
    pgd_clear(pgd_user1);
    ptb_clear(ptb0_user1);
    ptb_clear(ptb1_user1);
    ptb_clear(ptb2_user1);
    ptb_clear(ptb3_user1);
    ptb_clear(ptb704_user1);

    /* ---- PDE 0 ----*/

    // PDE0 correspond a 0..4MB donc on identity-map en supervisor
    // On a besoin de mapper car user1 doit pouvoir resoudre @ du handler intr_hdlr
    map_ptb_identity(ptb0_user1, 0, PG_KRN | PG_RW);
    pg_set_entry(&pgd_user1[0], PG_KRN | PG_RW, page_get_nr(ptb0_user1));

    /* ---- PDE 1 ----*/

    // PDE1 (4..8MB) : identity-map  du code USER (en ring 3)
    //PDE[1] en user (sinon impossible d’acceder ring3 au code user)
    pg_set_entry(&pgd_user1[1], PG_USR | PG_RW, page_get_nr(ptb1_user1));
    
    // Comme mes tables PGD, PTB, etc. sont dans accessibles physiquement par la meme PDE, 
    // J'ajuste au niveau des PTE pour que les @ physiques entre 0x0060 0000 a 0x0080 0000
    // soient accessibles uniquement en ring 0
    uint32_t base_frame = 1024;

    for (int i = 0; i < 1024; i++) {
        uint32_t flags = PG_RW;

        if (i < 512) flags |= PG_USR;  // 4MB..6MB = ring3
        else         flags |= PG_KRN;  // 6MB..8MB = ring0

        pg_set_entry(&ptb1_user1[i], flags, base_frame + (uint32_t)i);
    }

    /* ---- PDE 2 ----*/

    // PDE2 : 8..12MB : identity-map uniquement la pile ring3 user 1
    // pile ring3 user1 : 0x00800000 (index PTE 0)
    // On ne mappe pas en identity la page partagee a 0x00802000
    pg_set_entry(&pgd_user1[2], PG_USR | PG_RW, page_get_nr(ptb2_user1));
    map_page_in_ptb(ptb2_user1, (void*)0x00800000u, 0x00800000u, PG_USR | PG_RW);

    // Page partagee  : PA = 0x00802000, VA = USER1_SHARED_VA (0xb0001000)
    // PDE index de 0xb0001000 = 704, PTE index = 1
    pg_set_entry(&pgd_user1[pd32_get_idx(USER1_SHARED_VA)], PG_USR | PG_RW, page_get_nr(ptb704_user1));
    map_page_in_ptb(ptb704_user1, USER1_SHARED_VA, 0x00802000u, PG_USR | PG_RW);

    /* ---- PDE 3 ----*/

    // PDE3 : 12..16MB : on identity-map en supervisor pour les piles noyau
    map_ptb_identity(ptb3_user1, 3072, PG_KRN | PG_RW);
    pg_set_entry(&pgd_user1[3], PG_KRN | PG_RW, page_get_nr(ptb3_user1));

}
