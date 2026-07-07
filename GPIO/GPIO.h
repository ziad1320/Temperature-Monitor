#ifndef GPIO_H
#define GPIO_H

#include "../lib/std_types.h"

/*PortName*/
#define GPIO_A     'A'
#define GPIO_B     'B'
#define GPIO_C     'C'
#define GPIO_D     'D'

/*PinMode*/
#define GPIO_INPUT  0x00
#define GPIO_OUTPUT 0x01
#define GPIO_AF     0x02
#define GPIO_ANALOG 0x03

/*DefaultState*/
#define GPIO_PUSH_PULL  0x00
#define GPIO_OPEN_DRAIN 0x01

#define GPIO_NO_PULL_DOWN 0x00
#define GPIO_PULL_UP      0x01
#define GPIO_PULL_DOWN    0x02

/*Data*/
#define LOW      0x0
#define HIGH     0x1

#define OK  0x0
#define NOK 0x1

/* ========================================================================= */
/*                   Alternate Function (AF) Definitions                     */
/* ========================================================================= */
#define GPIO_AF0    0x00U  /* System / AF0 */
#define GPIO_AF1    0x01U  /* TIM1 / TIM2 */
#define GPIO_AF2    0x02U  /* TIM3 / TIM4 / TIM5 */
#define GPIO_AF3    0x03U  /* TIM9 / TIM10 / TIM11 */
#define GPIO_AF4    0x04U  /* I2C1 / I2C2 / I2C3 */
#define GPIO_AF5    0x05U  /* SPI1 / SPI2 / SPI3 / SPI4 */
#define GPIO_AF6    0x06U  /* SPI3 */
#define GPIO_AF7    0x07U  /* USART1 / USART2 */
#define GPIO_AF8    0x08U  /* USART6 */
#define GPIO_AF9    0x09U  /* I2C2 / I2C3 */
#define GPIO_AF10   0x0AU  /* OTG_FS */
#define GPIO_AF11   0x0BU  /* Reserved */
#define GPIO_AF12   0x0CU  /* SDIO */
#define GPIO_AF13   0x0DU  /* Reserved */
#define GPIO_AF14   0x0EU  /* Reserved */
#define GPIO_AF15   0x0FU  /* EVENTOUT */

void Gpio_Init(uint8 PortName, uint8 PinNumber, uint8 PinMode, uint8 DefaultState);
uint8 Gpio_WritePin(uint8 PortName, uint8 PinNumber, uint8 Data);
uint8 Gpio_ReadPin(uint8 PortName, uint8 PinNumber);
void Gpio_SetAF(uint8 PortName, uint8 PinNumber, uint8 AF);

#endif