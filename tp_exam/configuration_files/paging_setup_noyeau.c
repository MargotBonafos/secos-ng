/* GPLv2 (c) Airbus */
#include <debug.h>
#include <cr.h>
#include <pagemem.h>
#include <string.h>
#include <stdint.h>
#include "paging_setup.h"

// Adresses des tables paging noyau (choisis une zone différente de user1/user2)
#define K_PGD_PA   0x00600000u
#define K_PTB0_PA  0x00601000u  // PDE 0 : 0..4MB
#define K_PTB1_PA  0x00602000u  // PDE 1 : 4..8MB
#define K_PTB2_PA  0x00603000u  // PDE 2 : 8..12MB
#define K_PTB3_PA  0x00604000u  // PDE 3 : 12..16MB  (inclut les piles noyau 0x00c00000)

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

    pte32_t *ptb0 = (pte32_t*)K_PTB0_PA;
    pte32_t *ptb1 = (pte32_t*)K_PTB1_PA;
    pte32_t *ptb2 = (pte32_t*)K_PTB2_PA;
    pte32_t *ptb3 = (pte32_t*)K_PTB3_PA;

    // Nettoyage
    pgd_clear(pgd_kernel);
    ptb_clear(ptb0);
    ptb_clear(ptb1);
    ptb_clear(ptb2);
    ptb_clear(ptb3);

    /*
     * PDE 0 : 0..4MB (identity, supervisor)
     * base_frame = 0
     */
    map_ptb_identity(ptb0, 0, PG_KRN | PG_RW);
    pg_set_entry(&pgd_kernel[0], PG_KRN | PG_RW, page_get_nr(ptb0));

    /*
     * PDE 1 : 4..8MB (identity, supervisor)
     * base_frame = 0x00400000 >> 12 = 1024
     * (ça couvre aussi tes tables paging si tu les mets à 0x0060xxxx)
     */
    map_ptb_identity(ptb1, 1024, PG_KRN | PG_RW);
    pg_set_entry(&pgd_kernel[1], PG_KRN | PG_RW, page_get_nr(ptb1));

    /*
     * PDE 2 : 8..12MB (identity, supervisor)
     * base_frame = 0x00800000 >> 12 = 2048
     */
    map_ptb_identity(ptb2, 2048, PG_KRN | PG_RW);
    pg_set_entry(&pgd_kernel[2], PG_KRN | PG_RW, page_get_nr(ptb2));

    /*
     * PDE 3 : 12..16MB (identity, supervisor)
     * base_frame = 0x00C00000 >> 12 = 3072
     * (inclut les piles noyau user1/user2 à 0x00c00000 / 0x00c01000)
     */
    map_ptb_identity(ptb3, 3072, PG_KRN | PG_RW);
    pg_set_entry(&pgd_kernel[3], PG_KRN | PG_RW, page_get_nr(ptb3));

    // Debug utile
    debug("[paging] kernel PGD=%p PTB0=%p PTB1=%p PTB2=%p PTB3=%p\n",
          pgd_kernel, ptb0, ptb1, ptb2, ptb3);

    // Charger CR3
    set_cr3((uint32_t)pgd_kernel);
}