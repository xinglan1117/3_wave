/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "OLED.h"
#include "string.h"
#include "stdarg.h"
#include "stdio.h"
#include <math.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum {
    WAVE_SINE = 0,
    WAVE_SAW,
    WAVE_SQUARE
} WaveType;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define TICK_MS   10U
#define PI_F      3.14159265f
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
volatile uint8_t wave_cur         = WAVE_SINE;
volatile uint8_t wave_change_flag = 0;
volatile uint8_t rx_byte          = 0;

uint8_t  shown_wave  = 0xFF;
uint8_t  send_pending = 0;
uint32_t point_index  = 0;

static const float WAVE_PERIOD[3] = {1.0f, 0.5f, 1.5f};
static const float WAVE_AMP[3]    = {2.0f, 1.0f, 0.5f};
static const char *WAVE_NAME[3]   = {"Sine", "Sawtooth", "Square"};
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void Vofa_JustFloat(float *_data, uint8_t _num);
static float calc_wave(WaveType w, uint32_t idx);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_TIM2_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  OLED_init();
  HAL_TIM_Base_Start_IT(&htim2);
  HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    /* 1. 处理波形切换 */
        if (wave_change_flag)
        {
            wave_change_flag = 0;
            point_index = 0;
            send_pending = 1;
        }

        /* 2. 每 10ms 发一个波形点 */
        if (send_pending)
        {
            float v;
            float N;

            send_pending = 0;
            v = calc_wave((WaveType)wave_cur, point_index);
            Vofa_JustFloat(&v, 1);

            point_index++;
            N = WAVE_PERIOD[wave_cur] / (TICK_MS / 1000.0f);
            if (point_index >= (uint32_t)N)
                point_index = 0;
        }

        /* 3. 仅波形变化时刷新 OLED */
        if (shown_wave != wave_cur)
        {
            shown_wave = wave_cur;

            OLED_operate_gram(PEN_CLEAR);
            OLED_show_string(0, 0, (uint8_t *)"Wave:");
            OLED_show_string(0, 5, (uint8_t *)WAVE_NAME[wave_cur]);
            // OLED_printf(2, 0, "T:%0.1fs", WAVE_PERIOD[wave_cur]);
            // OLED_printf(3, 0, "A:%0.1f",  WAVE_AMP[wave_cur]);
            OLED_refresh_gram();
        }
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
/* ---------------- VOFA JustFloat 发送 ---------------- */
void Vofa_JustFloat(float *_data, uint8_t _num)
{
    uint8_t tempData[100];
    uint8_t temp_end[4] = {0, 0, 0x80, 0x7F};
    float   temp_copy[_num];

    memcpy(&temp_copy, _data, sizeof(float) * _num);
    memcpy(tempData, (uint8_t *)&temp_copy, sizeof(temp_copy));
    memcpy(&tempData[_num * 4], &temp_end[0], 4);

    HAL_UART_Transmit_IT(&huart1, tempData, (_num + 1) * 4);
}

/* ---------------- 波形计算 ---------------- */
static float calc_wave(WaveType w, uint32_t idx)
{
    float N  = WAVE_PERIOD[w] / (TICK_MS / 1000.0f);
    float ph = (float)idx / N;

    switch (w)
    {
        case WAVE_SINE:
            return WAVE_AMP[w] * sinf(2.0f * PI_F * ph);

        case WAVE_SAW:
            return WAVE_AMP[w] * (2.0f * ph - 1.0f);

        case WAVE_SQUARE:
            return (ph < 0.5f) ? WAVE_AMP[w] : -WAVE_AMP[w];

        default:
            return 0.0f;
    }
}

/* ---------------- 串口接收完成回调 ---------------- */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        uint8_t c = rx_byte;

        if (c >= '1' && c <= '3')
        {
            wave_cur = c - '1';
            wave_change_flag = 1;
        }

        HAL_UART_Receive_IT(&huart1, (uint8_t *)&rx_byte, 1);
    }
}

/* ---------------- TIM 周期回调 ---------------- */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2)
    {
        send_pending = 1;
    }
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
