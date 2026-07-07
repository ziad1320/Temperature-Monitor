#include "onewire.h"

// Reference the timer you configured in main.c for microsecond delays
extern TIM_HandleTypeDef htim10; 

// Microsecond delay using hardware timer
void Delay_us(uint16_t us) {
    __HAL_TIM_SET_COUNTER(&htim10, 0);
    while (__HAL_TIM_GET_COUNTER(&htim10) < us);
}

// Initialization sequence: Reset pulse followed by Presence pulse
uint8_t DS18B20_Start(void) {
    uint8_t presence = 0;
    
    // Pull line low for 480us
    HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_RESET);
    Delay_us(480);
    
    // Release line and wait 80us for sensor response
    HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_SET);
    Delay_us(80);
    
    // Check if the sensor pulled the line low (Presence Pulse)
    if (!(HAL_GPIO_ReadPin(DS18B20_PORT, DS18B20_PIN))) {
        presence = 1;
    }
    
    // Wait for the remainder of the timeslot
    Delay_us(400); 
    return presence;
}

void DS18B20_WriteBit(uint8_t bit) {
    if (bit) {
        // Write 1: Pull low for 1us, then release for 60us
        HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_RESET);
        Delay_us(1);
        HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_SET);
        Delay_us(60);
    } else {
        // Write 0: Pull low for 60us, then release
        HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_RESET);
        Delay_us(60);
        HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_SET);
        Delay_us(1); // Recovery time between bits
    }
}

void DS18B20_WriteByte(uint8_t data) {
    for (int i = 0; i < 8; i++) {
        DS18B20_WriteBit(data & 0x01); // LSB first
        data >>= 1;
    }
}

uint8_t DS18B20_ReadBit(void) {
    uint8_t bit = 0;
    
    // Pull low for 1us to initiate read timeslot
    HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_RESET);
    Delay_us(1);
    
    // Release and wait 10us to sample the line
    HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_SET);
    Delay_us(10);
    
    if (HAL_GPIO_ReadPin(DS18B20_PORT, DS18B20_PIN)) {
        bit = 1;
    }
    
    // Wait for the remainder of the 60us timeslot
    Delay_us(50);
    return bit;
}

uint8_t DS18B20_ReadByte(void) {
    uint8_t data = 0;
    for (int i = 0; i < 8; i++) {
        if (DS18B20_ReadBit()) {
            data |= (1 << i); // LSB first
        }
    }
    return data;
}