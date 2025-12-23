#include <pagemem.h>
#pragma once

void paging_setup_kernel(void);
void paging_setup_user1(void);
void paging_setup_user2(void);

uint32_t get_user1_pgd_addr(void);
uint32_t get_user2_pgd_addr(void);