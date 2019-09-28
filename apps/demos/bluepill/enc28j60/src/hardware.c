#include "stm32f1xx.h"
#include "locking.h"

#include "enc28j60.h"
#include "hardware.h"


lock_t spi_lock;
spi_largebuf_callback_t spi_largebuf_callback;

SPI_HandleTypeDef hspi1;


void enc28j60_init_spi(void)
{
  // Turn GPIOA Clock on
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  // Init CS1
  HAL_GPIO_Init(GPIOA, &(GPIO_InitTypeDef){
    .Pin  = GPIO_PIN_4,
    .Mode = GPIO_MODE_OUTPUT_PP,
    .Pull = GPIO_NOPULL,
    .Speed  = GPIO_SPEED_HIGH
  });
  // Init SCK, MISO, MOSI
  HAL_GPIO_Init(GPIOA, &(GPIO_InitTypeDef){
    .Pin  = GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7,
    .Mode = GPIO_MODE_AF_PP,
    .Pull = GPIO_NOPULL,
    .Speed  = GPIO_SPEED_HIGH
  });
  // Init RESET (PB6/SCL)
  HAL_GPIO_Init(GPIOB, &(GPIO_InitTypeDef){
    .Pin  = GPIO_PIN_6,
    .Mode = GPIO_MODE_OUTPUT_PP,
    .Pull = GPIO_NOPULL,
    .Speed  = GPIO_SPEED_HIGH
  });
  // Init WCL (PB7/SDA)
  HAL_GPIO_Init(GPIOB, &(GPIO_InitTypeDef){
    .Pin  = GPIO_PIN_7,
    .Mode = GPIO_MODE_INPUT,
    .Pull = GPIO_NOPULL,
    .Speed  = GPIO_SPEED_HIGH
  });
  // Init INT
  HAL_GPIO_Init(GPIOA, &(GPIO_InitTypeDef){
    .Pin  = GPIO_PIN_8,
    .Mode = GPIO_MODE_INPUT,
    .Pull = GPIO_NOPULL,
    .Speed  = GPIO_SPEED_HIGH
  });
  // Init PWM1 (LED) (PB9/SDA)
  HAL_GPIO_Init(GPIOB, &(GPIO_InitTypeDef){
    .Pin  = GPIO_PIN_9,
    .Mode = GPIO_MODE_OUTPUT_PP,
    .Pull = GPIO_NOPULL,
    .Speed  = GPIO_SPEED_HIGH
  });
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_SET);

  // Start non-selected, non-resetted
  ENC_SPI_Select(false);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);

  hspi1.Instance = SPI1;
  // SPI1 is part of the APB2 clock domain with 72 MHz.
  // A Baurateprescaler of 8 results in a spi clock of 9 MHz.
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
  hspi1.Init.Direction         = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize          = SPI_DATASIZE_8BIT;
  hspi1.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLED;
  hspi1.Init.CLKPhase          = SPI_PHASE_1EDGE;
  hspi1.Init.CLKPolarity       = SPI_POLARITY_LOW;
  hspi1.Init.Mode              = SPI_MODE_MASTER;
  hspi1.Init.NSS               = SPI_NSS_SOFT;
  hspi1.Init.TIMode            = SPI_TIMODE_DISABLED;
  hspi1.Init.FirstBit          = SPI_FIRSTBIT_MSB;
  
  // Turn SPI1 Clock on
  __HAL_RCC_SPI1_CLK_ENABLE();
  
  // Initialize SPI1
  HAL_SPI_Init(&hspi1);
}


/**
  * Implement SPI Slave selection and deselection. Must be provided by user code
  * param  select: true if the ENC28J60 slave SPI if selected, false otherwise
  * retval none
  */

void ENC_SPI_Select(bool select)
{
  if (select)
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
  else
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
}

/**
  * Implement SPI single byte send and receive.
  * The ENC28J60 slave SPI must already be selected and wont be deselected after transmission
  * Must be provided by user code
  * param  command: command or data to be sent to ENC28J60
  * retval answer from ENC28J60
  */

uint8_t ENC_SPI_SendWithoutSelection(uint8_t command)
{
  uint8_t to_send = command;
  uint8_t res = 0x00;
  HAL_SPI_TransmitReceive(&hspi1, &to_send, &res, 1, 1000);
  return res;
}

/**
  * Implement SPI single byte send and receive. Must be provided by user code
  * param  command: command or data to be sent to ENC28J60
  * retval answer from ENC28J60
  */

uint8_t ENC_SPI_Send(uint8_t command)
{
  ENC_SPI_Select(true);
  uint8_t res = ENC_SPI_SendWithoutSelection(command);
  ENC_SPI_Select(false);
  return res;
}

/**
  * Implement SPI buffer send and receive. Must be provided by user code
  * param  master2slave: data to be sent from host to ENC28J60, can be NULL if we only want to receive data from slave
  * param  slave2master: answer from ENC28J60 to host, can be NULL if we only want to send data to slave
  * NOTE: the two buffers may overlap
  * retval none
  */

void ENC_SPI_SendBuf(uint8_t *master2slave, uint8_t *slave2master, uint16_t bufferSize)
{
  ENC_SPI_Select(true);

  if (!master2slave)
    HAL_SPI_Receive(&hspi1, slave2master, bufferSize, 1000);
  else if (!slave2master)
    HAL_SPI_Transmit(&hspi1, master2slave, bufferSize, 1000);
  else
    HAL_SPI_TransmitReceive(&hspi1, master2slave, slave2master, bufferSize, 1000);

  ENC_SPI_Select(false);
}

// TODO
void ENC_SPI_SendLargeBuf(uint8_t *master2slave, uint8_t *slave2master, uint16_t bufferSize)
{
  ENC_SPI_SendBuf(master2slave, slave2master, bufferSize);
  if (spi_largebuf_callback)
    spi_largebuf_callback();
}
