#include "GPIO.h"
#include "GPIO_private.h"
#include "../lib/std_types.h"

#define GPIO_REG(PORT_ID, REG_ID)   ((volatile uint32 *) ((PORT_ID) + (REG_ID)))
uint32 addressMap[4] = {GPIOA_BASE_ADDR, GPIOB_BASE_ADDR, GPIOC_BASE_ADDR, GPIOD_BASE_ADDR};

void Gpio_Init(uint8 port_name, uint8 pin_number, uint8 pin_mode, uint8 default_state) {
    uint32 gpio_index = port_name - GPIO_A;
    GpioType* GpioDevice = (GpioType*) addressMap[gpio_index];

    GpioDevice -> GPIO_MODER &= ~(0x03 << (pin_number * 2));
    GpioDevice -> GPIO_MODER |= (pin_mode << (pin_number * 2));

    if (pin_mode == GPIO_INPUT) {
        GpioDevice -> GPIO_PUPDR &= ~(0x03 << (pin_number * 2));
        GpioDevice -> GPIO_PUPDR |= (default_state << (pin_number * 2));
    }else if (pin_mode == GPIO_OUTPUT) {
        GpioDevice -> GPIO_OTYPER &= ~(0x01 << pin_number);
        GpioDevice -> GPIO_OTYPER |= (default_state << pin_number);
    }
}

uint8 Gpio_WritePin(uint8 port_name, uint8 pin_number, uint8 data) {
    uint8 status = NOK;
    uint32 gpio_index = port_name - GPIO_A;
    GpioType* GpioDevice = (GpioType*) addressMap[gpio_index];

    if (((GpioDevice->GPIO_MODER & (0x03 << (pin_number * 2))) >> (pin_number * 2)) != GPIO_INPUT) {
        GpioDevice -> GPIO_ODR &= ~(0x01 << pin_number);
        GpioDevice -> GPIO_ODR |= (data << pin_number);
        status = OK;
    }
    return status;
}

uint8 Gpio_ReadPin(uint8 port_name, uint8 pin_number) {
    uint8 data = 0;
    uint32 gpio_index = port_name - GPIO_A;
    GpioType* GpioDevice = (GpioType*) addressMap[gpio_index];

    data = (GpioDevice -> GPIO_IDR >> pin_number) & 0x01;
    return data;
}

void Gpio_SetAF(uint8 port_name, uint8 pin_number, uint8 AF) {
    uint8 addressIndex = port_name - GPIO_A;
    GpioType* GpioDevice = (GpioType*) addressMap[addressIndex];

    if (pin_number < 8) {
        GpioDevice->GPIO_AFRL &= ~((uint32)0x0F << (pin_number * 4));
        GpioDevice->GPIO_AFRL |=  ((uint32)AF   << (pin_number * 4));
    } else {
        uint8 pos = pin_number - 8;
        GpioDevice->GPIO_AFRH &= ~((uint32)0x0F << (pos * 4));
        GpioDevice->GPIO_AFRH |=  ((uint32)AF   << (pos * 4));
    }
}