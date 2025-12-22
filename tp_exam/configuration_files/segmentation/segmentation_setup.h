#pragma once

void segmentation_setup_gdt(void);

extern seg_desc_t my_gdt[]; // Exposition de la GDT creer
