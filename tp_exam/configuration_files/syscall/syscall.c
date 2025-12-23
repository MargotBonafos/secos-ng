#include <intr.h>
#include <debug.h>

#include "syscall.h"


uint32_t syscall_hdlr(int_ctx_t *ctx)
{
    uint32_t user_ptr = ctx->gpr.eax.raw;  // On recupere registre eax passe par user2
                                          // et donc @ de compteur (cf sys_counter)

    uint32_t value = *(volatile uint32_t*)user_ptr;  // lecture dans la page partagee
    debug("[syscall] compteur=%u (ptr=0x%x)\n", value, user_ptr); // affichage par ring 0 du compteur 

    ctx->gpr.eax.raw = 0; // On met 0 dans eax

    // On revient a intr.c
    return (uint32_t)ctx;
}
