#pragma once
#include <stdint.h>

typedef struct {
  uint32_t kernel_base; // base - pile noyau de la tâche
  uint32_t kernel_esp;  // esp pile noyau 
  uint32_t cr3;         // CR3
} task_t;

extern task_t task_user1;
extern task_t task_user2;
extern task_t *current_task;