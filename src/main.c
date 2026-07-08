/**
 * @file    main.c
 * @brief   Non-blocking, interrupt-driven DS18B20 & ESP-01S telemetry system.
 */

#include "stm32f4xx_hal.h"
#include "onewire.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

/* --- Network & Configuration Macros --- */
#define WIFI_SSID             "YourSSID"
#define WIFI_PASSWORD         "YourPassword"
#define SERVER_IP             "192.168.1.100"
#define SERVER_PORT           80

/* --- Timing Macros (Milliseconds) --- */
#define TEMP_READ_INTERVAL_MS 10000
#define TEMP_CONV_DELAY_MS    800
#define ESP_TIMEOUT_MS        5000

/* --- Application State Machine --- */
typedef enum {
    STATE_INIT_MAC,
    STATE_WAIT_MAC,
    STATE_INIT_WIFI,
    STATE_WAIT_WIFI,
    STATE_IDLE,
    STATE_REQUEST_TEMP,
    STATE_WAIT_CONVERSION,
    STATE_READ_TEMP,
    STATE_TCP_CONNECT,
    STATE_WAIT_TCP,
    STATE_SEND_LEN,
    STATE_WAIT_PROMPT,
    STATE_SEND_PAYLOAD,
    STATE_WAIT_SEND_OK,
    STATE_ERROR
} AppState_t;

/* --- Local Peripheral Handles --- */
static TIM_HandleTypeDef htim10;
static UART_HandleTypeDef huart1;
static IWDG_HandleTypeDef hiwdg;

/* --- Interrupt-Driven UART Ring Buffer --- */
#define RX_BUF_SIZE 256
static volatile uint8_t rx_ring_buf[RX_BUF_SIZE];
static volatile uint16_t rx_head = 0;
static uint16_t rx_tail = 0;
static uint8_t rx_byte;

/* --- Static Global Buffers --- */
static char tx_buffer[128];
static float current_temperature = 0.0f;

/* --- Function Prototypes --- */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM10_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_IWDG_Init(void);
void Error_Handler(void);
bool UART_CheckResponse(const char *expected);

int main(void)
{
    /* Initialize the HAL Library */
    HAL_Init();

    /*
     * ====================================================================
     * PROTEUS VS REAL HARDWARE TOGGLE
     * ====================================================================
     * IF IN PROTEUS: Comment out SystemClock_Config() so it runs at 16MHz.
     * IF ON REAL HARDWARE: Leave it uncommented to run at 84MHz.
     * ====================================================================
     */
    // SystemClock_Config();

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_TIM10_Init();
    MX_USART1_UART_Init();
    MX_IWDG_Init();

    /* Pass the timer reference to the OneWire driver */
    Onewire_Init(&htim10);

    /* Start the microsecond timer for 1-Wire */
    if (HAL_TIM_Base_Start(&htim10) != HAL_OK) {
        Error_Handler();
    }

    /* Start UART Interrupt Reception */
    HAL_UART_Receive_IT(&huart1, (uint8_t*)&rx_byte, 1);

    /* State Machine Variables */
    AppState_t current_state = STATE_INIT_MAC;
    uint32_t state_timer = HAL_GetTick();

    /* Give the UART interrupt a moment to settle */
    HAL_Delay(100);

    while (1)
    {
        /* Pet the Watchdog */
        HAL_IWDG_Refresh(&hiwdg);

        uint32_t current_time = HAL_GetTick();

        switch (current_state)
        {
            case STATE_INIT_MAC:
                /* Allow ESP-01S time to boot, then configure */
                if (current_time - state_timer >= 2000)
                {
                    rx_tail = rx_head; // Flush buffer
                    HAL_UART_Transmit_IT(&huart1, (uint8_t*)"AT+CWMODE=1\r\n", 13);
                    state_timer = current_time;
                    current_state = STATE_WAIT_MAC;
                }
                break;

            case STATE_WAIT_MAC:
                if (UART_CheckResponse("\r\nOK\r\n")) {
                    current_state = STATE_INIT_WIFI;
                } else if (current_time - state_timer > ESP_TIMEOUT_MS) {
                    current_state = STATE_ERROR;
                }
                break;

            case STATE_INIT_WIFI:
            {
                sprintf(tx_buffer, "AT+CWJAP=\"%s\",\"%s\"\r\n", WIFI_SSID, WIFI_PASSWORD);
                HAL_UART_Transmit_IT(&huart1, (uint8_t*)tx_buffer, strlen(tx_buffer));
                state_timer = current_time;
                current_state = STATE_WAIT_WIFI;
                break;
            }

            case STATE_WAIT_WIFI:
                if (UART_CheckResponse("\r\nOK\r\n")) {
                    state_timer = current_time;
                    current_state = STATE_IDLE;
                } else if (current_time - state_timer > 15000) { // WiFi connection can take longer
                    current_state = STATE_ERROR;
                }
                break;

            case STATE_IDLE:
                /* Non-blocking wait for the next reading interval */
                if (current_time - state_timer >= TEMP_READ_INTERVAL_MS) {
                    current_state = STATE_REQUEST_TEMP;
                }
                break;

            case STATE_REQUEST_TEMP:
                if (DS18B20_Start())
                {
                    DS18B20_WriteByte(0xCC); // Skip ROM
                    DS18B20_WriteByte(0x44); // Convert T command

                    state_timer = current_time; // Record conversion start time
                    current_state = STATE_WAIT_CONVERSION;
                }
                else
                {
                    current_state = STATE_ERROR;
                }
                break;

            case STATE_WAIT_CONVERSION:
                /* Non-blocking delay: wait for the 12-bit conversion to finish */
                if (current_time - state_timer >= TEMP_CONV_DELAY_MS)
                {
                    current_state = STATE_READ_TEMP;
                }
                break;

            case STATE_READ_TEMP:
            {
                if (DS18B20_Start())
                {
                    DS18B20_WriteByte(0xCC); // Skip ROM
                    DS18B20_WriteByte(0xBE); // Read Scratchpad

                    uint8_t lsb = DS18B20_ReadByte();
                    uint8_t msb = DS18B20_ReadByte();

                    uint16_t raw_temp = (msb << 8) | lsb;
                    current_temperature = (float)raw_temp / 16.0f;

                    current_state = STATE_TCP_CONNECT;
                }
                else
                {
                    current_state = STATE_ERROR;
                }
                break;
            }

            case STATE_TCP_CONNECT:
                sprintf(tx_buffer, "AT+CIPSTART=\"TCP\",\"%s\",%d\r\n", SERVER_IP, SERVER_PORT);
                rx_tail = rx_head; // Flush buffer
                HAL_UART_Transmit_IT(&huart1, (uint8_t*)tx_buffer, strlen(tx_buffer));
                state_timer = current_time;
                current_state = STATE_WAIT_TCP;
                break;

            case STATE_WAIT_TCP:
                if (UART_CheckResponse("\r\nOK\r\n") || UART_CheckResponse("ALREADY CONNECTED")) {
                    current_state = STATE_SEND_LEN;
                } else if (current_time - state_timer > ESP_TIMEOUT_MS) {
                    current_state = STATE_ERROR;
                }
                break;

            case STATE_SEND_LEN:
            {
                /* Format the payload */
                sprintf(tx_buffer, "Temp: %.2f C\r\n", current_temperature);

                char len_cmd[32];
                sprintf(len_cmd, "AT+CIPSEND=%d\r\n", (int)strlen(tx_buffer));
                rx_tail = rx_head;
                HAL_UART_Transmit_IT(&huart1, (uint8_t*)len_cmd, strlen(len_cmd));

                state_timer = current_time;
                current_state = STATE_WAIT_PROMPT;
                break;
            }

            case STATE_WAIT_PROMPT:
                if (UART_CheckResponse(">")) {
                    current_state = STATE_SEND_PAYLOAD;
                } else if (current_time - state_timer > ESP_TIMEOUT_MS) {
                    current_state = STATE_ERROR;
                }
                break;

            case STATE_SEND_PAYLOAD:
                rx_tail = rx_head;
                HAL_UART_Transmit_IT(&huart1, (uint8_t*)tx_buffer, strlen(tx_buffer));
                state_timer = current_time;
                current_state = STATE_WAIT_SEND_OK;
                break;

            case STATE_WAIT_SEND_OK:
                if (UART_CheckResponse("SEND OK\r\n")) {
                    HAL_UART_Transmit_IT(&huart1, (uint8_t*)"AT+CIPCLOSE\r\n", 13);
                    state_timer = current_time;
                    current_state = STATE_IDLE; // Cycle complete
                } else if (current_time - state_timer > ESP_TIMEOUT_MS) {
                    current_state = STATE_ERROR;
                }
                break;

            case STATE_ERROR:
                /* Error recovery: wait, then restart state machine */
                if (current_time - state_timer >= ESP_TIMEOUT_MS) {
                    state_timer = current_time;
                    current_state = STATE_INIT_MAC;
                }
                break;
        }

        /* Power Management: Put CPU to sleep until next SysTick or UART interrupt */
        __WFI();
    }
}

/**
 * @brief Parses the circular buffer for an expected response.
 */
bool UART_CheckResponse(const char *expected)
{
    if (rx_head == rx_tail) return false;

    /* Build a linear string from the ring buffer for parsing */
    char temp[RX_BUF_SIZE + 1];
    uint16_t i = 0;
    uint16_t curr = rx_tail;

    while (curr != rx_head && i < RX_BUF_SIZE) {
        temp[i++] = (char)rx_ring_buf[curr];
        curr = (curr + 1) % RX_BUF_SIZE;
    }
    temp[i] = '\0';

    if (strstr(temp, expected) != NULL) {
        rx_tail = rx_head; // Consume buffer on success
        return true;
    }

    if (strstr(temp, "\r\nERROR\r\n") != NULL) {
        rx_tail = rx_head; // Consume buffer on error to prevent infinite hang
        return false;
    }

    return false;
}

/* --- Interrupt Callbacks --- */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        rx_ring_buf[rx_head] = rx_byte;
        rx_head = (rx_head + 1) % RX_BUF_SIZE;
        // Re-arm interrupt
        HAL_UART_Receive_IT(&huart1, (uint8_t*)&rx_byte, 1);
    }
}

void USART1_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart1);
}

void SysTick_Handler(void)
{
    HAL_IncTick();
}

void Error_Handler(void)
{
    /* Disable interrupts and spin in an infinite loop */
    __disable_irq();
    while (1) { }
}

/* --- Initialization Functions --- */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM = 16;
    RCC_OscInitStruct.PLL.PLLN = 336;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
    RCC_OscInitStruct.PLL.PLLQ = 7;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
        Error_Handler();
    }
}

static void MX_USART1_UART_Init(void)
{
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_9|GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart1) != HAL_OK) {
        Error_Handler();
    }

    /* Enable USART1 Interrupt in NVIC */
    HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
}

static void MX_TIM10_Init(void)
{
    __HAL_RCC_TIM10_CLK_ENABLE();

    htim10.Instance = TIM10;

    /*
     * ====================================================================
     * TIMER PRESCALER PROTEUS VS REAL HARDWARE
     * ====================================================================
     * IF IN PROTEUS (16MHz): Set Prescaler to 15
     * IF ON REAL HARDWARE (84MHz): Set Prescaler to 83
     * ====================================================================
     */
    htim10.Init.Prescaler = 15;

    htim10.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim10.Init.Period = 0xFFFF;
    htim10.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim10.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim10) != HAL_OK) {
        Error_Handler();
    }
}

static void MX_GPIO_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

static void MX_IWDG_Init(void)
{
    hiwdg.Instance = IWDG;
    hiwdg.Init.Prescaler = IWDG_PRESCALER_256;
    hiwdg.Init.Reload = 4095;
    if (HAL_IWDG_Init(&hiwdg) != HAL_OK) {
        Error_Handler();
    }
}

/* --- _sbrk implementation with Dynamic Stack Collision Protection --- */
void *_sbrk(int incr) {
    extern char end asm("end");
    static char *heap_ptr = 0;
    char *base;
    char stack_proxy;

    if (heap_ptr == 0) {
        heap_ptr = &end;
    }

    if ((heap_ptr + incr) >= &stack_proxy) {
        return (void *)-1;
    }

    base = heap_ptr;
    heap_ptr += incr;
    return base;
}