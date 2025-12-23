/* GPLv2 (c) Airbus */
#include <debug.h>
#include <cr.h>
#include <pagemem.h>
#include <string.h>
#include "paging_setup.h"

// Adresses des tables paging noyau (choisis une zone différente de user1/user2)
#define K_PGD_PA   0x00600000u
#define K_PTB0_PA  0x00601000u  // PDE 0 : 0..4MB
#define K_PTB1_PA  0x00602000u  // PDE 1 : 4..8MB
#define K_PTB2_PA  0x00603000u  // PDE 2 : 8..12MB
#define K_PTB3_PA  0x00604000u  // PDE 3 : 12..16MB

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

void paging_setup_kernel(void)
{
    pde32_t *pgd_kernel = (pde32_t*)K_PGD_PA;

    pte32_t *ptb0_kernel = (pte32_t*)K_PTB0_PA;
    pte32_t *ptb1_kernel = (pte32_t*)K_PTB1_PA;
    pte32_t *ptb2_kernel = (pte32_t*)K_PTB2_PA;
    pte32_t *ptb3_kernel = (pte32_t*)K_PTB3_PA;

    // Nettoyage
    pgd_clear(pgd_kernel);
    ptb_clear(ptb0_kernel);
    ptb_clear(ptb1_kernel);
    ptb_clear(ptb2_kernel);
    ptb_clear(ptb3_kernel);

    /*  --- PDE 0 : 0..4MB (identity & supervisor) --- */
    map_ptb_identity(ptb0_kernel, 0, PG_KRN | PG_RW);
    pg_set_entry(&pgd_kernel[0], PG_KRN | PG_RW, page_get_nr(ptb0_kernel));


    /*  --- PDE 1 : 4..8MB (identity & supervisor) --- */
    map_ptb_identity(ptb1_kernel, 1024, PG_KRN | PG_RW);
    pg_set_entry(&pgd_kernel[1], PG_KRN | PG_RW, page_get_nr(ptb1_kernel));


    /*  --- PDE 2 : 8..12MB (identity & supervisor) --- */
    map_ptb_identity(ptb2_kernel, 2048, PG_KRN | PG_RW);
    pg_set_entry(&pgd_kernel[2], PG_KRN | PG_RW, page_get_nr(ptb2_kernel));

    /*  --- PDE 2 : 12..16MB (identity & supervisor) --- */
    map_ptb_identity(ptb3_kernel, 3072, PG_KRN | PG_RW);
    pg_set_entry(&pgd_kernel[3], PG_KRN | PG_RW, page_get_nr(ptb3_kernel));

    // Affichage pour verif

    /*debug("[paging] kernel PGD=%p PTB0=%p PTB1=%p PTB2=%p PTB3=%p\n",
          pgd_kernel, ptb0_kernel, ptb1_kernel, ptb2_kernel, ptb3_kernel);*/

    // Charger CR3
    set_cr3((uint32_t)pgd_kernel);
}