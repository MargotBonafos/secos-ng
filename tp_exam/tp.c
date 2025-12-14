/* GPLv2 (c) Airbus */
#include <debug.h>
#include <segmem.h>

// Tache user 1
void tache_user1() {
	printf("Je suis user 1\n");
	//asm volatile ("mov %eax, %cr0");
}

// Tache user 2
void tache_user2() {
	printf("Je suis user 2\n");
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


void tp() {

	/*****  Creation de la GDT *****/ 

	//Creation de notre GDT//
	
	seg_desc_t my_gdt[7];

    my_gdt[0].raw = 0ULL;

    //GDT[]

	// Variable qui contient addr et taille de la GDT
    gdt_reg_t gdtr_value;

    // la fonction get_gdtr met la valeur du registre GDTR dans gdtr_value
    get_gdtr(gdtr_value);

    // Affichage des valeurs gdtr_value (et donc implicitement des valeurs du registre GDTR)
    printf("taille de la GDT : %hu \n", gdtr_value.limit);
    printf("addresse de la GDT : %lu  \n", gdtr_value.addr);

    // Affichage du contenu de la GDT
    print_gdt_content(gdtr_value);

    // Affichage des selecteurs de segments
    printf("DS : %hu \n", get_ds());
    printf("CS : %hu \n", get_seg_sel(cs));

    printf("SS : %hu \n", get_ss());
    printf("ES : %hu \n", get_es());
    printf("FS : %hu \n", get_fs());
    printf("GS : %hu \n", get_gs());




}
