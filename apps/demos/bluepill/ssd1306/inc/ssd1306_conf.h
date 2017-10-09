// Include the right header file for the device used
#include "stm32f1xx_hal.h"

// I2C handle for HAL code
#define SSD1306_I2C_PORT        hi2c

// I2C address
#define SSD1306_I2C_ADDR        0x78

// SSD1306 size in pixels
#define SSD1306_WIDTH           128
#define SSD1306_HEIGHT          64
