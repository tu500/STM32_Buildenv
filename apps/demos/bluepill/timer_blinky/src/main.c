#include "stm32f1xx.h"

static void TIM4_Config(void);
static void SystemClock_Config(void);
static void Error_Handler(void);

/* Variables used for timer */
uint16_t PrescalerValue = 0;
TIM_HandleTypeDef htim4;

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(int argc, char const *argv[])
{
  HAL_Init();
  SystemClock_Config();

  /* Enable the GPIO_LED Clock */
  __HAL_RCC_GPIOC_CLK_ENABLE();

  /* Configure the GPIO_LED pin */
  HAL_GPIO_Init(GPIOC, &(GPIO_InitTypeDef){
      .Pin    = GPIO_PIN_13,
      .Mode   = GPIO_MODE_OUTPUT_PP,
      .Pull   = GPIO_NOPULL,
      .Speed  = GPIO_SPEED_HIGH,
    });

  /* Reset PIN to switch off the LED */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);

  /* Enable timer clock */
  __HAL_RCC_TIM4_CLK_ENABLE();

  /* Configure timer interrupt */
  HAL_NVIC_SetPriority((TIM4_IRQn), 0x00, 0);
  HAL_NVIC_EnableIRQ((TIM4_IRQn));

  /* Configure timer */
  TIM4_Config();


  while(1)
  {
  }

  return 0;
}

void TIM4_IRQHandler(void)
{
  /* TIM Update event */
  if(__HAL_TIM_GET_FLAG(&htim4, TIM_FLAG_UPDATE) != RESET)
  {
    if(__HAL_TIM_GET_IT_SOURCE(&htim4, TIM_IT_UPDATE) !=RESET)
    {
      __HAL_TIM_CLEAR_IT(&htim4, TIM_IT_UPDATE);

      /* Toggle LED pin */
      HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    }
  }
}

static void TIM4_Config(void)
{
  /* -----------------------------------------------------------------------
  TIM4 Configuration: Output Compare Timing Mode:

  In this example TIM4 input clock (TIM4CLK) is set to 2 * APB1 clock (PCLK1),
  since APB1 prescaler is different from 1 (APB1 Prescaler = 4, see system_stm32f4xx.c file).
  TIM4CLK = 2 * PCLK1
  PCLK1 = HCLK / 4
  => TIM4CLK = 2*(HCLK / 4) = HCLK/2 = SystemCoreClock/2

  To get TIM4 counter clock at 2 KHz, the prescaler is computed as follows:
  Prescaler = (TIM4CLK / TIM4 counter clock) - 1
  Prescaler = (84 MHz/(2 * 2 KHz)) - 1 = 41999

  To get TIM4 output clock at 1 Hz, the period (ARR)) is computed as follows:
  ARR = (TIM4 counter clock / TIM4 output clock) - 1
  = 1999
  ----------------------------------------------------------------------- */

  /* Compute the prescaler value */
  PrescalerValue = (uint16_t) ((SystemCoreClock /2) / 2000) - 1;

  /* Time base configuration */
  htim4.Instance               = TIM4;
  htim4.Init.Period            = 1999;
  htim4.Init.Prescaler         = PrescalerValue;
  htim4.Init.ClockDivision     = 0;
  htim4.Init.CounterMode       = TIM_COUNTERMODE_UP;
  htim4.Init.RepetitionCounter = 0;

  if(HAL_TIM_Base_Init(&htim4) != HAL_OK)
  {
    /* Initialization Error */
    Error_Handler();
  }

  if(HAL_TIM_Base_Start_IT(&htim4) != HAL_OK)
  {
    /* Initialization Error */
    Error_Handler();
  }
}

/**
 * @brief  System Clock Configuration
 *         The system Clock is configured as follow :
 *            System Clock source            = PLL (HSE)
 *            SYSCLK(Hz)                     = 72000000
 *            HCLK(Hz)                       = 72000000
 *            AHB Prescaler                  = 1
 *            APB1 Prescaler                 = 2
 *            APB2 Prescaler                 = 1
 *            HSE Frequency(Hz)              = 8000000
 *            HSE PREDIV                     = 1
 *            PLLMUL                         = RCC_PLL_MUL9 (9)
 *            Flash Latency(WS)              = 2
 * @param  None
 * @retval None
 */
void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;

  /* Enable HSE Oscillator and activate PLL with HSE as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2
   clocks dividers */
  RCC_ClkInitStruct.ClockType =
      (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
static void Error_Handler(void)
{
  /* Turn LED5 on */
  while(1)
  {
  }
}

#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif
