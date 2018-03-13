#ifndef WS2812B_H
#define WS2812B_H

#define WS2812B_LED_COUNT 4

void ws2812b_init(void);
void ws2812b_start(uint32_t count);
void ws2812b_fill_dma_buffer(uint32_t index, uint8_t r, uint8_t g, uint8_t b);

#endif /* end of include guard: WS2812B_H */
