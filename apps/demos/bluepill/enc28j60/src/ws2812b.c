#include <stdbool.h>

#include "stm32f1xx.h"

#include "ws2812b.h"

typedef struct
{
  uint8_t prefix; // sometimes the hardware eats the first sample...
  uint8_t pwm_buffer[24*WS2812B_LED_COUNT];
  uint8_t suffix[2];
} pwm_buffer_struct_t;

static pwm_buffer_struct_t pwm_buffer_struct = { 0, {0}, {0} };
#define pwm_buffer (pwm_buffer_struct.pwm_buffer)

static TIM_HandleTypeDef htim1;
static TIM_OC_InitTypeDef sConfigTim1;
static DMA_HandleTypeDef hdma_tim1;

static void ws2812b_dma_init(void);
static void ws2812b_tim1_init(void);
static void ws2812b_fill_dma_buffer_byte(uint8_t *dest, uint8_t value);
static void Error_Handler(void);


/*************************
 *                       *
 *    Main Interface     *
 *                       *
 *************************/

void ws2812b_init(void)
{
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /* Configure the GPIO_LED pin */
  HAL_GPIO_Init(GPIOB, &(GPIO_InitTypeDef){
      .Pin    = GPIO_PIN_13,
      .Mode   = GPIO_MODE_AF_PP,
      .Pull   = GPIO_NOPULL,
      .Speed  = GPIO_SPEED_HIGH,
    });

  /* Configure dma */
  ws2812b_dma_init();

  /* Configure timer */
  ws2812b_tim1_init();

  __HAL_TIM_ENABLE(&htim1);
}

void ws2812b_start(uint32_t count)
{
  if (count > WS2812B_LED_COUNT)
    Error_Handler();

  /* Set stop data */
  pwm_buffer[24*count + 0] = 0;
  pwm_buffer[24*count + 1] = 0;

  /* Enable the DMA channel */
  HAL_DMA_Start_IT(&hdma_tim1, (uint32_t)(&pwm_buffer_struct), (uint32_t)&htim1.Instance->CCR1, 1+24*count + 2);

  __HAL_TIM_ENABLE_DMA(&htim1, TIM_DMA_UPDATE);
  /* __HAL_TIM_ENABLE(&htim1); */
}

void ws2812b_fill_dma_buffer(uint32_t index, uint8_t r, uint8_t g, uint8_t b)
{
  ws2812b_fill_dma_buffer_byte(&pwm_buffer[24*index],    g);
  ws2812b_fill_dma_buffer_byte(&pwm_buffer[24*index+8],  r);
  ws2812b_fill_dma_buffer_byte(&pwm_buffer[24*index+16], b);
}


/*************************
 *                       *
 *   Buffer handling     *
 *                       *
 *************************/

#define WS2812B_PULSE_HIGH ((uint32_t)(2*30-1))
#define WS2812B_PULSE_LOW ((uint32_t)(30-1))

static const uint32_t ws2812b_pwm_lut[16] = {
  (WS2812B_PULSE_LOW<<0)  | (WS2812B_PULSE_LOW<<8)  | (WS2812B_PULSE_LOW<<16)  | (WS2812B_PULSE_LOW<<24),
  (WS2812B_PULSE_LOW<<0)  | (WS2812B_PULSE_LOW<<8)  | (WS2812B_PULSE_LOW<<16)  | (WS2812B_PULSE_HIGH<<24),
  (WS2812B_PULSE_LOW<<0)  | (WS2812B_PULSE_LOW<<8)  | (WS2812B_PULSE_HIGH<<16) | (WS2812B_PULSE_LOW<<24),
  (WS2812B_PULSE_LOW<<0)  | (WS2812B_PULSE_LOW<<8)  | (WS2812B_PULSE_HIGH<<16) | (WS2812B_PULSE_HIGH<<24),
  (WS2812B_PULSE_LOW<<0)  | (WS2812B_PULSE_HIGH<<8) | (WS2812B_PULSE_LOW<<16)  | (WS2812B_PULSE_LOW<<24),
  (WS2812B_PULSE_LOW<<0)  | (WS2812B_PULSE_HIGH<<8) | (WS2812B_PULSE_LOW<<16)  | (WS2812B_PULSE_HIGH<<24),
  (WS2812B_PULSE_LOW<<0)  | (WS2812B_PULSE_HIGH<<8) | (WS2812B_PULSE_HIGH<<16) | (WS2812B_PULSE_LOW<<24),
  (WS2812B_PULSE_LOW<<0)  | (WS2812B_PULSE_HIGH<<8) | (WS2812B_PULSE_HIGH<<16) | (WS2812B_PULSE_HIGH<<24),
  (WS2812B_PULSE_HIGH<<0) | (WS2812B_PULSE_LOW<<8)  | (WS2812B_PULSE_LOW<<16)  | (WS2812B_PULSE_LOW<<24),
  (WS2812B_PULSE_HIGH<<0) | (WS2812B_PULSE_LOW<<8)  | (WS2812B_PULSE_LOW<<16)  | (WS2812B_PULSE_HIGH<<24),
  (WS2812B_PULSE_HIGH<<0) | (WS2812B_PULSE_LOW<<8)  | (WS2812B_PULSE_HIGH<<16) | (WS2812B_PULSE_LOW<<24),
  (WS2812B_PULSE_HIGH<<0) | (WS2812B_PULSE_LOW<<8)  | (WS2812B_PULSE_HIGH<<16) | (WS2812B_PULSE_HIGH<<24),
  (WS2812B_PULSE_HIGH<<0) | (WS2812B_PULSE_HIGH<<8) | (WS2812B_PULSE_LOW<<16)  | (WS2812B_PULSE_LOW<<24),
  (WS2812B_PULSE_HIGH<<0) | (WS2812B_PULSE_HIGH<<8) | (WS2812B_PULSE_LOW<<16)  | (WS2812B_PULSE_HIGH<<24),
  (WS2812B_PULSE_HIGH<<0) | (WS2812B_PULSE_HIGH<<8) | (WS2812B_PULSE_HIGH<<16) | (WS2812B_PULSE_LOW<<24),
  (WS2812B_PULSE_HIGH<<0) | (WS2812B_PULSE_HIGH<<8) | (WS2812B_PULSE_HIGH<<16) | (WS2812B_PULSE_HIGH<<24)
};

static void ws2812b_fill_dma_buffer_byte(uint8_t *dest, uint8_t value)
{
  // lut algorithm
  *((uint32_t*)dest)     = ws2812b_pwm_lut[value >> 4];
  *((uint32_t*)(dest+4)) = ws2812b_pwm_lut[value & 0x0f];

  // bitwise algorithm
  //for (int i=8; i; i--)
  //{
  //if (value & 0x80)
  //  *dest = WS2812B_PULSE_HIGH;
  //else
  //  *dest = WS2812B_PULSE_LOW;
  //
  //dest++;
  //value <<= 1;
  //}
}


/*************************
 *                       *
 *         TIM1          *
 *                       *
 *************************/

static void ws2812b_tim1_init(void)
{
  /* Enable timer clock */
  __HAL_RCC_TIM1_CLK_ENABLE();

  /* Configure timer interrupt */
  HAL_NVIC_SetPriority((TIM1_UP_IRQn), 0x00, 0);
  HAL_NVIC_EnableIRQ((TIM1_UP_IRQn));


  /* Time base configuration */
  htim1.Instance               = TIM1;
  htim1.Init.Prescaler         = 0; // -> 36 MHz
  htim1.Init.Period            = 90; // -> 800 kHz
  htim1.Init.ClockDivision     = 0;
  htim1.Init.CounterMode       = TIM_COUNTERMODE_UP;
  htim1.Init.RepetitionCounter = 0;

  if(HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    /* Initialization Error */
    Error_Handler();
  }

  sConfigTim1.OCMode = TIM_OCMODE_PWM1;
  sConfigTim1.OCIdleState = TIM_OCIDLESTATE_SET;
  sConfigTim1.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigTim1.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigTim1.Pulse = 0;

  /* Output Compare PWM1 Mode configuration: Channel1 */
  if(HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigTim1, TIM_CHANNEL_1) != HAL_OK)
  {
    /* Initialization Error */
    Error_Handler();
  }

  // workaround limited HAL driver
  TIM1->CCER = TIM_CCER_CC1NE;
  TIM1->CCMR1 = 0x0068;

  TIM1->EGR = 0x01; // update preloaded registers

  __HAL_TIM_MOE_ENABLE(&htim1);
  /* __HAL_TIM_ENABLE_IT(&htim1, TIM_IT_UPDATE); */
}

/* UNUSED */
/* void TIM1_UP_IRQHandler(void) */
/* { */
/*   #<{(| TIM Update event |)}># */
/*   if(__HAL_TIM_GET_FLAG(&htim1, TIM_FLAG_UPDATE) != RESET) */
/*   { */
/*     if(__HAL_TIM_GET_IT_SOURCE(&htim1, TIM_IT_UPDATE) !=RESET) */
/*     { */
/*       __HAL_TIM_CLEAR_IT(&htim1, TIM_IT_UPDATE); */
/*  */
/*       #<{(| Toggle LED pin |)}># */
/*       #<{(| HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13); |)}># */
/*     } */
/*   } */
/* } */


/*************************
 *                       *
 *         DMA           *
 *                       *
 *************************/

static void errorcallback(DMA_HandleTypeDef *hdma);
static void cpltcallback(DMA_HandleTypeDef *hdma);

static void ws2812b_dma_init(void)
{
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* Configure DMA hardware */
  hdma_tim1.Instance = DMA1_Channel5;
  hdma_tim1.Parent = &htim1; // need to save i2c handle for use callbacks (HAL internal)
  hdma_tim1.Init.Direction           = DMA_MEMORY_TO_PERIPH;
  hdma_tim1.Init.PeriphInc           = DMA_PINC_DISABLE;
  hdma_tim1.Init.MemInc              = DMA_MINC_ENABLE;
  hdma_tim1.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
  hdma_tim1.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
  hdma_tim1.Init.Mode                = DMA_NORMAL;
  hdma_tim1.Init.Priority            = DMA_PRIORITY_LOW;
  HAL_DMA_Init(&hdma_tim1);

  /* Enable DMA interrupt */
  HAL_NVIC_SetPriority((DMA1_Channel5_IRQn), 0x00, 0);
  HAL_NVIC_EnableIRQ((DMA1_Channel5_IRQn));

  /* Set the DMA Period elapsed callback */
  hdma_tim1.XferCpltCallback = cpltcallback;
  /* Set the DMA error callback */
  hdma_tim1.XferErrorCallback = errorcallback;
}

static void errorcallback(DMA_HandleTypeDef *hdma)
{
}

static void cpltcallback(DMA_HandleTypeDef *hdma)
{
}

void DMA1_Channel5_IRQHandler(void)
{
  /* Call HAL interrupt handler */
  HAL_DMA_IRQHandler(&hdma_tim1);
}


/*************************
 *                       *
 *     Error Handler     *
 *                       *
 *************************/

static void Error_Handler(void)
{
  while(1);
}
