#include "stm32f1xx.h"

#include "ssd1306.h"

I2C_HandleTypeDef hi2c;
DMA_HandleTypeDef hdma_i2ctx;

static void SystemClock_Config(void);
static void Error_Handler(void);

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(int argc, char const *argv[])
{
  HAL_Init();
  SystemClock_Config();


  /* Enable peripheral clocks */
  /* There seems to be problems when enabling clocks just before initializing
   * the i2c hardware, so do it here */
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_I2C1_CLK_ENABLE();
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* Configure I2C pins (PB6, PB7) */
  HAL_GPIO_Init(GPIOB, &(GPIO_InitTypeDef){
      .Pin    = GPIO_PIN_6 | GPIO_PIN_7,
      .Mode   = GPIO_MODE_AF_OD, // alternate function
      .Pull   = GPIO_PULLUP,
      .Speed  = GPIO_SPEED_HIGH,
    });

  /* Configure I2C hardware */
  hi2c.Instance = I2C1;
  hi2c.hdmatx = &hdma_i2ctx; // need to specify the dma handler
  hi2c.Init.ClockSpeed      = 400000;
  hi2c.Init.DutyCycle       = I2C_DUTYCYCLE_2;
  hi2c.Init.OwnAddress1     = 0;
  hi2c.Init.AddressingMode  = I2C_ADDRESSINGMODE_7BIT;
  hi2c.Init.DualAddressMode = I2C_DUALADDRESS_DISABLED;
  hi2c.Init.OwnAddress2     = 0;
  hi2c.Init.GeneralCallMode = I2C_GENERALCALL_DISABLED;
  hi2c.Init.NoStretchMode   = I2C_NOSTRETCH_DISABLED;
  HAL_I2C_Init(&hi2c);


  /* Configure DMA hardware */
  hdma_i2ctx.Instance = DMA1_Channel6;
  hdma_i2ctx.Parent = &hi2c; // need to save i2c handle for use callbacks (HAL internal)
  hdma_i2ctx.Init.Direction           = DMA_MEMORY_TO_PERIPH;
  hdma_i2ctx.Init.PeriphInc           = DMA_PINC_DISABLE;
  hdma_i2ctx.Init.MemInc              = DMA_MINC_ENABLE;
  hdma_i2ctx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
  hdma_i2ctx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
  hdma_i2ctx.Init.Mode                = DMA_NORMAL;
  hdma_i2ctx.Init.Priority            = DMA_PRIORITY_LOW;
  HAL_DMA_Init(&hdma_i2ctx);

  /* Enable DMA interrupt */
  HAL_NVIC_SetPriority((DMA1_Channel6_IRQn), 0x00, 0);
  HAL_NVIC_EnableIRQ((DMA1_Channel6_IRQn));


  /* Wait for display to power up */
  HAL_Delay(100);
  /* while(!HAL_I2C_IsDeviceReady(&hi2c, )); */

  /* Initialize display */
  ssd1306_Init();

  /* Wait until current command is complete */
  while(!ssd1306_IsReady());


  HAL_Delay(1000);

  /* White display */
  ssd1306_Fill(SSD1306_COLOR_White);
  ssd1306_UpdateScreen();

  while(!ssd1306_IsReady());


  while(1)
  {
    HAL_Delay(1000);

    /* Print test message */
    ssd1306_Fill(SSD1306_COLOR_White);
    ssd1306_SetCursor(23,23);
    ssd1306_WriteString("Test", Font_11x18, SSD1306_COLOR_Black);

    ssd1306_UpdateScreen();
    while(!ssd1306_IsReady());


    HAL_Delay(1000);

    /* Print another test message */
    ssd1306_Fill(SSD1306_COLOR_Black);
    ssd1306_SetCursor(23,23);
    ssd1306_WriteString("Invert", Font_11x18, SSD1306_COLOR_White);

    ssd1306_UpdateScreen();
    while(!ssd1306_IsReady());
  }

  return 0;
}

void DMA1_Channel6_IRQHandler(void)
{
  /* Call HAL interrupt handler */
  HAL_DMA_IRQHandler(&hdma_i2ctx);
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
