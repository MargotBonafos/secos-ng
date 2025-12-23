/* GPLv2 (c) Airbus */
#include <debug.h>
#include <segmem.h>
#include <string.h>

#include "configuration_files/tss/tss_setup.h"
#include <configuration_files/segmentation/segmentation_setup.h> // Pour utilisation de my_gdt

tss_t TSS;

void tss_setup(void){

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
}