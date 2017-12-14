#include <stdbool.h>
#include <stdio.h>

#include "stm32f1xx.h"
#include "uip.h"

#include "hardware.h"
#include "enc28j60.h"

#include "mqtt.h"
#include "autoc4.h"

static void SystemClock_Config(void);
static void Error_Handler(void);

ENC_HandleTypeDef henc;
uint8_t MacAddress[6] = { 0xac, 0xde, 0x48, 0x23, 0x23, 0x10 };

void do_nothing(void){}
void uip_setup(void);
void uip_loop(void);

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

  enc28j60_init_spi();
  uip_setup();

  henc.Init.InterruptEnableBits = EIE_RXERIE | EIE_TXERIE | EIE_TXIE | EIE_LINKIE | EIE_PKTIE | EIE_INTIE;
  henc.Init.DuplexMode = ETH_MODE_HALFDUPLEX;
  henc.Init.MACAddr = &MacAddress[0];
  henc.Init.ChecksumMode = ETH_CHECKSUM_BY_HARDWARE;
  henc.RxFrameInfos.buffer = &uip_buf[0];

  printf("Setup\r\n");

  bool b = ENC_Start(&henc);
  ENC_SetMacAddr(&henc);

  printf("ENC Started\r\n");

  if (b)
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

  autoc4_init();

  char const * autosub[] = {
    "topic1",
    NULL
  };
  mqtt_connection_config_t mqtt_config;
  mqtt_config.client_id = "clientid";
  mqtt_config.user = NULL;
  mqtt_config.pass = NULL;
  mqtt_config.will_topic = NULL;
  uip_ipaddr(mqtt_config.target_ip, 192,168,0,5);
  /* uip_ipaddr(mqtt_config.target_ip, 172,23,23,110); */
  mqtt_config.auto_subscribe_topics = autosub;

  /* mqtt_set_connection_config(&mqtt_config); */

  while(1)
  {
    /* HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_11)); */

    if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_11) == GPIO_PIN_RESET)
    {
      ENC_IRQCheckInterruptFlags(&henc);
    }

    HAL_Delay(10);
    mqtt_periodic();
    autoc4_periodic();
    uip_loop();
  }

  return 0;
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
