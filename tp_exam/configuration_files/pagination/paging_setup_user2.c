#include <debug.h>
#include <cr.h>
#include <pagemem.h>
#include <string.h>
#include "paging_setup.h"

extern uint32_t __kernel_start__, __kernel_end__;
extern uint32_t __user_start__,   __user_end__;
extern uint32_t __user2_stack_base__, __user2_stack_end__;
extern uint32_t __shared_base__, __shared_end__;
extern uint32_t __kernel_stack_user1_base__, __kernel_stack_user1_end__;
extern uint32_t __kernel_stack_user2_base__, __kernel_stack_user2_end__;

#define USER2_SHARED_VA     ((void*)0x03a0c000u)   /* PDE=13, PTE=524 */

/* Adresses des tables user2 */
#define U2_PGD_PA     0x00620000u
#define U2_PTB0_PA    0x00621000u  /* PDE 0 */
#define U2_PTB1_PA    0x00622000u  /* PDE 1 */
#define U2_PTB2_PA    0x00623000u  /* PDE 2 */
#define U2_PTB3_PA    0x00624000u  /* PDE 3 */

#define U2_PTB13_PA  0x00625000u  /* PDE 13 : shared VA */

static inline void ptb_clear(pte32_t *ptb) {
    memset((void*)ptb, 0, PAGE_SIZE);
}
static inline void pgd_clear(pde32_t *pgd) {
    memset((void*)pgd, 0, PAGE_SIZE);
}

static inline void map_ptb_identity(pte32_t *ptb, uint32_t base_frame, uint32_t flags) {
    for (int i = 0; i < 1024; i++) {
        pg_set_entry(&ptb[i], flags, base_frame + (uint32_t)i);
    }
}

static inline void map_page_in_ptb(pte32_t *ptb, void *va, uint32_t pa, uint32_t flags) {
    int pti = pt32_get_idx(va);
    pg_set_entry(&ptb[pti], flags, page_get_nr(pa));
}

void paging_setup_user2(void)
{
    pde32_t *pgd_user2 = (pde32_t*)U2_PGD_PA;

    pte32_t *ptb0_user2   = (pte32_t*)U2_PTB0_PA;
    pte32_t *ptb1_user2   = (pte32_t*)U2_PTB1_PA;
    pte32_t *ptb2_user2   = (pte32_t*)U2_PTB2_PA;
    pte32_t *ptb3_user2   = (pte32_t*)U2_PTB3_PA;
    pte32_t *ptb13_user2 = (pte32_t*)U2_PTB13_PA;

    /* Remise à zéro */
    pgd_clear(pgd_user2);
    ptb_clear(ptb0_user2);
    ptb_clear(ptb1_user2);
    ptb_clear(ptb2_user2);
    ptb_clear(ptb3_user2);
    ptb_clear(ptb13_user2);

    /* ---- PDE 0 ---- */

    // 0..4MB : identity-map en mode supervisor
    map_ptb_identity(ptb0_user2, 0, PG_KRN | PG_RW);
    pg_set_entry(&pgd_user2[0], PG_KRN | PG_RW, page_get_nr(ptb0_user2));

    /* ---- PDE 1 ---- */

    // 4..8MB, identity map pour code users (4..6MB) et ring0 pour tables (6..8MB) 
    pg_set_entry(&pgd_user2[1], PG_USR | PG_RW, page_get_nr(ptb1_user2));

    uint32_t base_frame = 1024;

    for (int i = 0; i < 1024; i++) {
        uint32_t flags = PG_RW;

        if (i < 512) flags |= PG_USR;  // 4MB..6MB = ring3
        else flags |= PG_KRN;  // 6MB..8MB = ring0

        pg_set_entry(&ptb1_user2[i], flags, base_frame + (uint32_t)i);
    }

    /* ---- PDE 2 ---- */
    // 8..12MB : on mappe que la pile ring3 user2 (0x00801000)
    pg_set_entry(&pgd_user2[2], PG_USR | PG_RW, page_get_nr(ptb2_user2));
    map_page_in_ptb(ptb2_user2, (void*)0x00801000u, 0x00801000u, PG_USR | PG_RW);

    /* ---- PDE 3 ---- */
    // 12..16MB : identity-map en mode supervisor (piles noyau à 0x00c00000)
    map_ptb_identity(ptb3_user2, 3072, PG_KRN | PG_RW);
    pg_set_entry(&pgd_user2[3], PG_KRN | PG_RW, page_get_nr(ptb3_user2));

    /* ---- PDE 13 ---- */
    // Configuration de VA user 2 -> @physique page partagee
    pg_set_entry(&pgd_user2[pd32_get_idx(USER2_SHARED_VA)], PG_USR | PG_RW, page_get_nr(ptb13_user2));
    map_page_in_ptb(ptb13_user2, USER2_SHARED_VA, 0x00802000u, PG_USR | PG_RW);


    /* Affichage pour verif */
    debug("[paging] user2: PGD=%p shared_va=%p", pgd_user2,USER2_SHARED_VA);
}
