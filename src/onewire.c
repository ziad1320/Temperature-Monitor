#include "onewire.h"

// Encapsulated timer handle (no longer a global extern)
static TIM_HandleTypeDef *ow_timer = NULL;

void Onewire_Init(TIM_HandleTypeDef *timer) {
    ow_timer = timer;
}

void Delay_us(uint16_t us) {
    if (ow_timer == NULL) return;
    __HAL_TIM_SET_COUNTER(ow_timer, 0);
    while (__HAL_TIM_GET_COUNTER(ow_timer) < us);
}

uint8_t DS18B20_Start(void) {
    uint8_t presence = 0;
    HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_RESET);
    Delay_us(480);
    HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_SET);
    Delay_us(80);
    if (!(HAL_GPIO_ReadPin(DS18B20_PORT, DS18B20_PIN))) {
        presence = 1;
    }
    Delay_us(400);
    return presence;
}

void DS18B20_WriteBit(uint8_t bit) {
    if (bit) {
        HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_RESET);
        Delay_us(1);
        HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_SET);
        Delay_us(60);
    } else {
        HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_RESET);
        Delay_us(60);
        HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_SET);
        Delay_us(1);
    }
}

void DS18B20_WriteByte(uint8_t data) {
    for (int i = 0; i < 8; i++) {
        DS18B20_WriteBit(data & 0x01);
        data >>= 1;
    }
}

uint8_t DS18B20_ReadBit(void) {
    uint8_t bit = 0;
    HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_RESET);
    Delay_us(1);
    HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_SET);
    Delay_us(10);
    if (HAL_GPIO_ReadPin(DS18B20_PORT, DS18B20_PIN)) {
        bit = 1;
    }
    Delay_us(50);
    return bit;
}

uint8_t DS18B20_ReadByte(void) {
    uint8_t data = 0;
    for (int i = 0; i < 8; i++) {
        if (DS18B20_ReadBit()) {
            data |= (1 << i);
        }
    }
    return data;
}