#ifndef ONEWIRE_H
#define ONEWIRE_H

#include "stm32f4xx_hal.h"

// Define your DS18B20 Port and Pin here
#define DS18B20_PORT GPIOA
#define DS18B20_PIN  GPIO_PIN_1

// Function Prototypes
void Onewire_Init(TIM_HandleTypeDef *timer);
void Delay_us(uint16_t us);
uint8_t DS18B20_Start(void);
void DS18B20_WriteBit(uint8_t bit);
void DS18B20_WriteByte(uint8_t data);
uint8_t DS18B20_ReadBit(void);
uint8_t DS18B20_ReadByte(void);

#endif /* ONEWIRE_H */