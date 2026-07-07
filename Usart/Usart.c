#include "Usart.h"
#include "../GPIO/Gpio.h"
#include "../lib/std_types.h"

void Usart2_Init(void) {
    /* 1. Configure GPIO Pins for USART2 (PA2 = TX, PA3 = RX) */
    Gpio_Init(GPIO_A, 2, GPIO_AF, GPIO_PUSH_PULL);
    Gpio_Init(GPIO_A, 3, GPIO_AF, GPIO_PUSH_PULL);

    /* Set Alternate Function 7 (AF7) for both pins to connect them to USART2 */
    Gpio_SetAF(GPIO_A, 2, GPIO_AF7);
    Gpio_SetAF(GPIO_A, 3, GPIO_AF7);

    /* 2. Configure Baud Rate (9600 baud @ 16 MHz APB1 Clock)
     * Formula: 16,000,000 / (16 * 9600) = 104.166
     * Mantissa = 104 (0x68), Fraction = 0.166 * 16 = ~3 (0x03) */
    USART2->BRR = 0x0683;

    /* 3. Configure Control Register 1 (CR1) */
    /* Clear register to enforce defaults: 8 data bits, 1 start bit, no parity */
    USART2->CR1 = 0;

    /* Enable Transmitter, Receiver, and the USART peripheral itself */
    USART2->CR1 |= (1UL << CR1_TE) | (1UL << CR1_RE) | (1UL << CR1_UE);
}

void Usart2_TransmitByte(uint8 Byte) {
    /* Wait ONLY until the Transmit Data Register is Empty (TXE) */
    /* This prevents the millisecond CPU freeze caused by waiting for TC */
    while (!((USART2->SR >> SR_TXE) & 1U)) {
        /* Poll */
    }

    /* Write the data to trigger transmission */
    USART2->DR = Byte;
}

void Usart2_TransmitString(const char *Str) {
    uint32 i = 0;
    while (Str[i] != '\0') {
        Usart2_TransmitByte((uint8)Str[i]);
        i++;
    }
}