#ifndef NVIC_H
#define NVIC_H

#include "../lib/std_types.h"

/**
 * @brief Enables a specific Interrupt Request (IRQ) in the NVIC.
 * @param IrqNumber The IRQ number (0 to 81 for STM32F401).
 */
void Nvic_EnableIrq(uint8 IrqNumber);

/**
 * @brief Disables a specific Interrupt Request (IRQ) in the NVIC.
 * @param IrqNumber The IRQ number.
 */
void Nvic_DisableIrq(uint8 IrqNumber);

#endif /* NVIC_H */
