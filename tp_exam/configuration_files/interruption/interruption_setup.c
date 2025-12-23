/* GPLv2 (c) Airbus */
#include <intr.h>
#include <pic.h>
#include <io.h>

#include "configuration_files/interruption/interruption_setup.h"

// L'EOI PIC est déjà géré dans idt.s [ack_pic1/ack_pic2]

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

    out(0x36, PIT_CMD);
    out((uint8_t)(div & 0xFF), PIT_CH0);
    out((uint8_t)((div >> 8) & 0xFF), PIT_CH0);
}

static void pic_unmask_irq0_only(void)
{
    // n'active que IRQ0 only
    out(0xFE, PIC_IMR(PIC1));

    // Masque tout en mode slave
    out(0xFF, PIC_IMR(PIC2));
}

void interruption_setup(uint32_t hz)
{
    // Desactive les interruptions (dont irq0) pendant la configuration
    cli(); 

    // Charger et initialiser l'IDT 
    intr_init();

    // Remappe les PIC (utilisation de pic.h)
    pic_init();

    // Autoriser uniquement IRQ0
    pic_unmask_irq0_only();

    // Configure la frequence du timer et donc des interruptions
    pit_init(hz);
}

/*
void activate_irq0(){
    // Reactive les interruptions (dont irq0) apres la configuration
    sti();
}
*/