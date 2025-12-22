/* GPLv2 (c) Airbus */
#include <stdint.h>
#include <intr.h>
#include <pic.h>
#include <io.h>

#include "configuration_files/interruption/interruption_setup.h"

/*
 * interruption_setup.c
 *
 * Rôle:
 *  - Charger l'IDT (intr_init)
 *  - Initialiser le PIC (remap IRQ 0..15 -> 32..47)
 *  - Unmask IRQ0 (timer)
 *  - Programmer le PIT pour générer des ticks IRQ0
 *  - Activer les interruptions (sti)
 *
 * Notes:
 *  - L'EOI PIC est déjà géré dans idt.s via ack_pic1/ack_pic2 (donc PAS d'EOI ici).
 *  - Assure-toi que intr_hdlr(ctx) traite bien le vector 32 (IRQ0).
 */

static inline void cli(void) { __asm__ volatile("cli"); }
static inline void sti(void) { __asm__ volatile("sti"); }

/* PIT (8253/8254) */
#define PIT_FREQ_HZ 1193182u
#define PIT_CMD     0x43
#define PIT_CH0     0x40

static void pit_init(uint32_t hz)
{
    if (hz == 0) hz = 100;
    uint32_t div = PIT_FREQ_HZ / hz;

    /* ch0, lobyte/hibyte, mode 3 (square wave), binary */
    out(0x36, PIT_CMD);
    out((uint8_t)(div & 0xFF), PIT_CH0);
    out((uint8_t)((div >> 8) & 0xFF), PIT_CH0);
}

static void pic_unmask_irq0_only(void)
{
    /* Master IMR: enable IRQ0 only (bit0=0), mask others (bits=1) => 0xFE */
    out(0xFE, PIC_IMR(PIC1));

    /* Slave IMR: mask all => 0xFF */
    out(0xFF, PIC_IMR(PIC2));
}

/*
 * API simple:
 * - hz: fréquence du timer (ex: 100 ou 1000)
 */
void interruption_setup(uint32_t hz)
{
    cli();

    /* 1) IDT */
    intr_init();

    /* 2) PIC (remap) */
    pic_init();

    /* 3) Autoriser IRQ0 */
    pic_unmask_irq0_only();

    /* 4) PIT */
    pit_init(hz);

    /* 5) Enable CPU interrupts */
    sti();
}
