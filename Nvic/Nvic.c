#include "Nvic.h"

/* 
 * NVIC ISER (Interrupt Set-Enable Registers) Base Address.
 * Writing a 1 to these registers enables the interrupt.
 */
#define NVIC_ISER_BASE  0xE000E100UL

/* 
 * NVIC ICER (Interrupt Clear-Enable Registers) Base Address.
 * Writing a 1 to these registers disables the interrupt.
 */
#define NVIC_ICER_BASE  0xE000E180UL

void Nvic_EnableIrq(uint8 IrqNumber) {
    volatile uint32* iser = (volatile uint32*)NVIC_ISER_BASE;
    /* Divide by 32 to find the right register, modulo 32 for the bit */
    iser[IrqNumber / 32] = (1UL << (IrqNumber % 32));
}

void Nvic_DisableIrq(uint8 IrqNumber) {
    volatile uint32* icer = (volatile uint32*)NVIC_ICER_BASE;
    icer[IrqNumber / 32] = (1UL << (IrqNumber % 32));
}
