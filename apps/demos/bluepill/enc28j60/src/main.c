#include <stdbool.h>
#include <stdio.h>

#include "stm32f1xx.h"
#include "uip.h"

#include "hardware.h"
#include "enc28j60.h"
#include "locking.h"
#include "uip_interface.h"

#include "mqtt.h"
#include "autoc4.h"

#include "timer.h"
#include "ws2812b.h"

static void SystemClock_Config(void);
static void Error_Handler(void);

ENC_HandleTypeDef henc;
uint8_t MacAddress[6] = { 0xac, 0xde, 0x48, 0x23, 0x23, 0xff };

void ENC_ScheduleReinit(ENC_HandleTypeDef *handle);
void ENC_ScheduleReinit_static(void);
void ENC_PollReinit(ENC_HandleTypeDef *handle);

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

  ws2812b_init();
  ws2812b_fill_dma_buffer(0, 0, 0, 0);
  ws2812b_fill_dma_buffer(1, 0, 0, 0);
  ws2812b_fill_dma_buffer(2, 0, 0, 0);
  ws2812b_fill_dma_buffer(3, 0, 0, 0);

  HAL_Delay(1000);
  ws2812b_start(4);

  enc28j60_init_spi();
  uip_setup();

  henc.Init.InterruptEnableBits = EIE_RXERIE | EIE_TXERIE | EIE_TXIE | EIE_LINKIE | EIE_PKTIE | EIE_INTIE;
  henc.Init.DuplexMode = ETH_MODE_HALFDUPLEX;
  henc.Init.MACAddr = &MacAddress[0];
  henc.Init.ChecksumMode = ETH_CHECKSUM_BY_HARDWARE;
  henc.rxState.buffer = &uip_buf[0];

  printf("Setup\r\n");

  bool b = ENC_Start(&henc);
  ENC_SetMacAddr(&henc);

  printf("ENC Started\r\n");

  if (b)
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

  autoc4_init();

  struct timer ws2812b_refresh_timer;
  timer_set(&ws2812b_refresh_timer, CLOCK_SECOND * 1);

  struct timer periodic_timer;
  timer_set(&periodic_timer, CLOCK_SECOND/100);

  struct timer reinit;
  timer_set(&reinit, CLOCK_SECOND * 60);

  while(1)
  {
    /* HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_11)); */

    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_8) == GPIO_PIN_RESET)
    {
      ENC_IRQCheckInterruptFlags(&henc);
    }

    ENC_PollReinit(&henc);
    ENC_HandlePendingEvents(&henc);
    ENC_PollTxSetup(&henc);
    ENC_PollReadPacket(&henc);
    ENC_Transmit(&henc);

    if(timer_expired(&periodic_timer))
    {
      mqtt_periodic();
      autoc4_periodic();
      timer_reset(&periodic_timer);
    }

    if(timer_expired(&reinit))
    {
      // reinit the ENC chip every minute without an MQTT connection
      if (!mqtt_is_connected())
        ENC_ScheduleReinit(&henc);
      timer_restart(&reinit);
    }

    uip_loop();

    if(timer_expired(&ws2812b_refresh_timer))
    {
      ws2812b_start(4);
      timer_reset(&ws2812b_refresh_timer);
    }
  }

  return 0;
}

void ENC_ScheduleReinit_static(void)
{
  ENC_ScheduleReinit(&henc);
}

void ENC_ScheduleReinit(ENC_HandleTypeDef *handle)
{
  handle->reinit_scheduled = true;
}

void ENC_PollReinit(ENC_HandleTypeDef *handle)
{
  if (!handle->reinit_scheduled)
    return;

  if (handle->txState.stage != ENC_TX_STAGE_FREE)
    return;

  if (!lock_acquire(&spi_lock))
    return;

  printf("Reiniting ENC\r\n");
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);

  bool b = ENC_Start(&henc);
  ENC_SetMacAddr(&henc);

  handle->reinit_scheduled = false;
  lock_release(&spi_lock);
  lock_release(&uip_buf_lock);

  printf("ENC Started\r\n");

  if (b)
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
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
