#include <stdbool.h>

//  Deze library is door Olivier Van den Eede 2016 geschreven en aangepast voor gebruik met
//  Stm32 microcontrollers en maakt gebruik van de HAL-i2c library's.
//
//  Deze library is gemaakt om gebruik te kunnen maken van een ssd1306 gestuurd oled display.
//  Voor het gebruik moeten zeker de onderstaande defines juist ingesteld worden.
//  Zoals de gebruikte i2c poort en de groote van het scherm.
//
//  De library maakt gebruik van 2 files (fonts.c/h) Hierin staan 3 fonts beschreven.
//  Deze fonts kunnen zo gebruikt worden:   - Font_7x10
//                      - Font_11x18
//                      - Font_16x26

#ifndef ssd1306
#define ssd1306

#include "ssd1306_conf.h"
#include "ssd1306_fonts.h"


// Raw pointer to the frame buffer
// is of type `uint8_t [SSD1306_WIDTH * SSD1306_HEIGHT / 8]`
#define SSD1306_Buffer (SSD1306_frame_buffer_acc.frame_buffer)

typedef enum {
  SSD1306_COLOR_Black = 0x00, /* Black color, no pixel */
  SSD1306_COLOR_White = 0x01  /* Pixel is set. Color depends on LCD */
} SSD1306_COLOR;


// Public functions
void ssd1306_Init(void);
void ssd1306_Fill(SSD1306_COLOR color);
void ssd1306_UpdateScreen(void);
void ssd1306_DrawPixel(uint8_t x, uint8_t y, SSD1306_COLOR color);
char ssd1306_WriteChar(char ch, SSD1306_FontDef Font, SSD1306_COLOR color);
char ssd1306_WriteString(char* str, SSD1306_FontDef Font, SSD1306_COLOR color);
void ssd1306_SetCursor(uint8_t x, uint8_t y);
bool ssd1306_IsReady(void);

// Internal
struct SSD1306_frame_buffer_accelerator
{
  uint8_t control_byte;
  uint8_t frame_buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8];
};

extern struct SSD1306_frame_buffer_accelerator SSD1306_frame_buffer_acc;

void ssd1306_tx_complete_callback(void);

#endif
