#include "ssd1306.h"

struct SSD1306_frame_buffer_accelerator SSD1306_frame_buffer_acc;


/*******************
 *                 *
 *  Internal Data  *
 *                 *
 *******************/

typedef enum {
  SSD1306_STATE_RESET,
  SSD1306_STATE_CONFIGURING,
  SSD1306_STATE_INITIALIZED,
  SSD1306_STATE_READY,
  SSD1306_STATE_SCREEN_UPDATING,
} SSD1306_STATE;

// Display State
typedef struct {
  uint16_t CurrentX;
  uint16_t CurrentY;
  uint8_t Inverted;
  uint8_t State;
} SSD1306_t;

// Display status
static SSD1306_t SSD1306;


/***********************
 *                     *
 *  HAL customization  *
 *                     *
 ***********************/

// Used i2c handle
extern I2C_HandleTypeDef SSD1306_I2C_PORT;

// Write raw data out i2c interface
static void ssd1306_WriteBulk(uint16_t i2c_address, const uint8_t* data, uint32_t size)
{
  HAL_I2C_Master_Transmit_DMA(&SSD1306_I2C_PORT, i2c_address, (uint8_t*) data, size);
}

void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
  ssd1306_tx_complete_callback();
}

//// Low level SSD1306 command function
//static void ssd1306_WriteCommand(uint8_t command)
//{
//  HAL_I2C_Mem_Write(&SSD1306_I2C_PORT, SSD1306_I2C_ADDR, 0x00, 1, &command, 1, 10);
//}


/*******************
 *                 *
 *  SSD1306 Logic  *
 *                 *
 *******************/

// Initialize display
void ssd1306_Init(void)
{
  static const uint8_t ssd1306_config_commands[] =
  {
    0x00,             // control byte (command and continuation)
    0xAE,             // display off
    0x20, 0x00,       // set memory addressing mode (horizontal addressing)
    0x21, 0x00, 0x7F, // column 0 to 127
    0x22, 0x00, 0x07, // page 0 to 7
    0xC8,             // set COM output scan direction
    0x40,             // set start line address
    0x81, 0xFF,       // set contrast control register
    0xA1,             // set segment re-map 0 to 127
    0xA6,             // set normal display (as opposed to inverted)
    0xA8, 0x3F,       // set multiplex ratio (1 to 64)
    0xA4,             // 0xA4: output follows RAM content; 0xA5: output ignores RAM content
    0xD3, 0x00,       // set display offset (no offset)
    0xD5, 0xF0,       // set display clock divide ratio/oscillator frequency
    0xD9, 0x22,       // set pre-charge period
    0xDA, 0x12,       // set com pins hardware configuration
    0xDB, 0x20,       // set vcomh (0x20,0.77xVcc)
    0x8D, 0x14,       // set DC-DC enable
    0xAF,             // turn on SSD1306 panel
  };

  SSD1306.State = SSD1306_STATE_CONFIGURING;

  ssd1306_WriteBulk(
      SSD1306_I2C_ADDR,
      &ssd1306_config_commands[0],
      sizeof(ssd1306_config_commands)
    );

  // Initialize frame buffer
  ssd1306_Fill(SSD1306_COLOR_Black);

  // Set default values
  SSD1306.CurrentX = 0;
  SSD1306.CurrentY = 0;

  SSD1306_frame_buffer_acc.control_byte = 0x40;
}

// Called when a started i2c transfer is completed
void ssd1306_tx_complete_callback(void)
{
  switch (SSD1306.State)
  {
    case SSD1306_STATE_CONFIGURING:
      SSD1306.State = SSD1306_STATE_INITIALIZED;
      ssd1306_UpdateScreen();
      break;

    case SSD1306_STATE_SCREEN_UPDATING:
      SSD1306.State = SSD1306_STATE_READY;
      break;

    default:
      break;
  }
}

// Fill complete frame buffer with one color
void ssd1306_Fill(SSD1306_COLOR color)
{
  /* Set memory */
  for (uint32_t i = 0; i < sizeof(SSD1306_Buffer); i++)
  {
    SSD1306_Buffer[i] = (color == SSD1306_COLOR_Black) ? 0x00 : 0xFF;
  }
}

// Write frame buffer to display
void ssd1306_UpdateScreen(void)
{
  if (SSD1306.State != SSD1306_STATE_READY && SSD1306.State != SSD1306_STATE_INITIALIZED)
    return;

  SSD1306.State = SSD1306_STATE_SCREEN_UPDATING;
  ssd1306_WriteBulk(
      SSD1306_I2C_ADDR,
      &SSD1306_frame_buffer_acc.control_byte,
      sizeof(SSD1306_frame_buffer_acc)
    );
}

// Set a single pixel in the frame buffer
void ssd1306_DrawPixel(uint8_t x, uint8_t y, SSD1306_COLOR color)
{
  if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT)
  {
    // Out of range
    return;
  }

  // Possibly invert color
  if (SSD1306.Inverted)
  {
    color = (SSD1306_COLOR)!color;
  }

  // Set pixel
  if (color == SSD1306_COLOR_White)
  {
    SSD1306_Buffer[x + (y / 8) * SSD1306_WIDTH] |= 1 << (y % 8);
  }
  else
  {
    SSD1306_Buffer[x + (y / 8) * SSD1306_WIDTH] &= ~(1 << (y % 8));
  }
}

// Write a single character to the frame buffer, given a font
char ssd1306_WriteChar(char ch, SSD1306_FontDef Font, SSD1306_COLOR color)
{
  uint32_t i, b, j;

  // Kijken of er nog plaats is op deze lijn
  if (SSD1306_WIDTH <= (SSD1306.CurrentX + Font.FontWidth) ||
    SSD1306_HEIGHT <= (SSD1306.CurrentY + Font.FontHeight))
  {
    // Er is geen plaats meer
    return 0;
  }

  // We gaan door het font
  for (i = 0; i < Font.FontHeight; i++)
  {
    b = Font.data[(ch - 32) * Font.FontHeight + i];
    for (j = 0; j < Font.FontWidth; j++)
    {
      if ((b << j) & 0x8000)
      {
        ssd1306_DrawPixel(SSD1306.CurrentX + j, (SSD1306.CurrentY + i), (SSD1306_COLOR) color);
      }
      else
      {
        ssd1306_DrawPixel(SSD1306.CurrentX + j, (SSD1306.CurrentY + i), (SSD1306_COLOR)!color);
      }
    }
  }

  // De huidige positie is nu verplaatst
  SSD1306.CurrentX += Font.FontWidth;

  // We geven het geschreven char terug voor validatie
  return ch;
}

// Write a string to the frame buffer, given a font
char ssd1306_WriteString(char* str, SSD1306_FontDef Font, SSD1306_COLOR color)
{
  // We schrijven alle char tot een nulbyte
  while (*str)
  {
    if (ssd1306_WriteChar(*str, Font, color) != *str)
    {
      // Het karakter is niet juist weggeschreven
      return *str;
    }

    // Volgende char
    str++;
  }

  // Alles gelukt, we sturen dus 0 terug
  return *str;
}

// Set text cursor position
void ssd1306_SetCursor(uint8_t x, uint8_t y)
{
  /* Set write pointers */
  SSD1306.CurrentX = x;
  SSD1306.CurrentY = y;
}

// Return whether the display driver is ready to send commands / update the screen
bool ssd1306_IsReady(void)
{
  return SSD1306.State == SSD1306_STATE_READY;
}
