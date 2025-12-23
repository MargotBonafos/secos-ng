#include <debug.h>
#include <segmem.h>
#include "segmentation_setup.h"


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

seg_desc_t my_gdt[11];

void segmentation_setup_gdt(void){

    my_gdt[0].raw = 0ULL;

    //GDT[1] : Code ring 0 - noyau 
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

    // Pile noyau ring 0 - User 1
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

    // Pile noyau ring 0 - User 2
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

    // Pile noyau ring 0
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

    // Affichage du contenu de la GDT
    debug(" --- affichage de la GDT ---\n\n");
    print_gdt_content(gdtr_of_my_gdt);
    debug("\n\n");
    
    //Mettre a jour les selecteurs de segments
    set_cs(gdt_krn_seg_sel(1));
    set_ds(gdt_krn_seg_sel(9));
    set_ss(gdt_krn_seg_sel(9));

}