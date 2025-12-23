#include <intr.h>

#pragma once

#define VADDR_COUNTER_USER1 ((volatile uint32_t*)0xb0001000u)   // ta VA partagée user1
#define VADDR_COUNTER_USER2 ((volatile uint32_t*)0x03a0c000u)   // ta VA partagée user2

#define SYS_COUNTER  1u

uint32_t syscall_hdlr(int_ctx_t *ctx);
