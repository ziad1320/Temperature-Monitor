#ifndef USART_H
#define USART_H

#include "../lib/std_types.h"

typedef struct {
    volatile uint32 SR;   /* 0x00 - Status register */
    volatile uint32 DR;   /* 0x04 - Data register */
    volatile uint32 BRR;  /* 0x08 - Baud rate register */
    volatile uint32 CR1;  /* 0x0C - Control register 1 */
    volatile uint32 CR2;  /* 0x10 - Control register 2 */
    volatile uint32 CR3;  /* 0x14 - Control register 3 */
    volatile uint32 GTPR; /* 0x18 - Guard time and prescaler */
} UsartType;

#define USART2_BASE_ADDR 0x40004400UL
#define USART2           ((UsartType *)USART2_BASE_ADDR)

/* Register Bit Definitions */
#define SR_TXE  7U  /* Transmit data register empty */
#define CR1_UE  13U /* USART enable */
#define CR1_TE  3U  /* Transmitter enable */
#define CR1_RE  2U  /* Receiver enable */

/**
 * @brief Initializes USART2 on PA2 (TX) and PA3 (RX) at 9600 baud.
 * Used for PC serial communication and data logging.
 */
void Usart2_Init(void);

/**
 * @brief Transmits a single byte over UART.
 * Non-blocking (only waits for TXE, not TC).
 */
void Usart2_TransmitByte(uint8 Byte);

/**
 * @brief Transmits a complete null-terminated string over UART.
 */
void Usart2_TransmitString(const char *Str);

#endif