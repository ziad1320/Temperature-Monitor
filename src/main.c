#include "stm32f4xx_hal.h"
#include "onewire.h"
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

/* --- Peripheral Handles --- */
TIM_HandleTypeDef htim10;
UART_HandleTypeDef huart1;

/* --- Function Prototypes --- */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM10_Init(void);
static void MX_USART1_UART_Init(void);
void ESP01_SendCommand(const char *cmd);

int main(void)
{
    /* Initialize the HAL Library */
    HAL_Init();

    /* Configure the System Clock to 84 MHz */
    SystemClock_Config();

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_TIM10_Init();
    MX_USART1_UART_Init();

    /* Start the microsecond timer */
    HAL_TIM_Base_Start(&htim10);

    uint8_t temp_byte1, temp_byte2;
    uint16_t raw_temp;
    float temperature;
    char uart_buffer[100];

    /* Allow ESP-01S time to boot */
    HAL_Delay(2000);

    /* Basic ESP-01S Network Setup */
    // Put ESP in Station Mode
    ESP01_SendCommand("AT+CWMODE=1\r\n");
    HAL_Delay(500);
    // Connect to WiFi (Replace with actual SSID and Password)
    ESP01_SendCommand("AT+CWJAP=\"YourSSID\",\"YourPassword\"\r\n");
    HAL_Delay(5000);

    while (1)
    {
        /* 1. Request Temperature Conversion */
        if (DS18B20_Start())
        {
            DS18B20_WriteByte(0xCC); // Skip ROM (Sends command to all sensors, fine if only 1 is connected)
            DS18B20_WriteByte(0x44); // Convert T command
        }

        // Wait for conversion to complete (12-bit resolution requires ~750ms)
        HAL_Delay(800);

        /* 2. Read Temperature Data */
        if (DS18B20_Start())
        {
            DS18B20_WriteByte(0xCC); // Skip ROM
            DS18B20_WriteByte(0xBE); // Read Scratchpad

            temp_byte1 = DS18B20_ReadByte(); // LSB
            temp_byte2 = DS18B20_ReadByte(); // MSB

            // Combine bytes and calculate temperature
            raw_temp = (temp_byte2 << 8) | temp_byte1;
            temperature = (float)raw_temp / 16.0;

            /* 3. Send Data via ESP-01S */
            // Prepare the HTTP GET request or TCP payload string
            sprintf(uart_buffer, "Temp: %.2f C\r\n", temperature);

            // Example TCP connection to a database server
            ESP01_SendCommand("AT+CIPSTART=\"TCP\",\"192.168.1.100\",80\r\n");
            HAL_Delay(1000);

            char send_cmd[30];
            sprintf(send_cmd, "AT+CIPSEND=%d\r\n", (int)strlen(uart_buffer));
            ESP01_SendCommand(send_cmd);
            HAL_Delay(100);

            ESP01_SendCommand(uart_buffer);
            HAL_Delay(1000);

            ESP01_SendCommand("AT+CIPCLOSE\r\n");
        }
        else
        {
            // Sensor not detected
            ESP01_SendCommand("Sensor Error\r\n");
        }

        /* Read every 10 seconds */
        HAL_Delay(10000);
    }
}

/* --- ESP-01S Helper Function --- */
void ESP01_SendCommand(const char *cmd)
{
    HAL_UART_Transmit(&huart1, (uint8_t*)cmd, strlen(cmd), HAL_MAX_DELAY);
}

/* --- Initialization Functions --- */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    // Configure internal oscillator (HSI) to generate 84MHz
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
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}

static void MX_USART1_UART_Init(void)
{
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    // PA9 = TX, PA10 = RX
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
    HAL_UART_Init(&huart1);
}

static void MX_TIM10_Init(void)
{
    __HAL_RCC_TIM10_CLK_ENABLE();

    // Timer runs at APB2 clock (84 MHz). Prescaler 84-1 gives 1 MHz (1us tick)
    htim10.Instance = TIM10;
    htim10.Init.Prescaler = 83;
    htim10.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim10.Init.Period = 0xFFFF;
    htim10.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim10.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_Base_Init(&htim10);
}

static void MX_GPIO_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Configure PA1 for DS18B20 Open-Drain Output
    GPIO_InitStruct.Pin = DS18B20_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DS18B20_PORT, &GPIO_InitStruct);
}

void SysTick_Handler(void)
{
    HAL_IncTick();
}

/* --- _sbrk implementation to satisfy linker --- */
void *_sbrk(int incr) {
    extern char end asm("end");
    static char *heap_ptr = 0;
    char *base;

    if (heap_ptr == 0) {
        heap_ptr = &end;
    }
    base = heap_ptr;
    heap_ptr += incr;
    return base;
}