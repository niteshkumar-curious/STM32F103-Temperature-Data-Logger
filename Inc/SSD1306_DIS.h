#ifndef SSD1306_DIS_H_
#define SSD1306_DIS_H_

#include <stdint.h>

void SSD1306_Init(void);
void OLED_Display_clear(void);
void OLED_Display_Number(uint8_t x, uint8_t y, int32_t num);
void OLED_Display_String(uint8_t x, uint8_t y, const char *str);
void SSD1306_updateScreen(void);
void OLED_Display_Float(uint8_t x, uint8_t y, float num);
void OLED_Display_Char(uint8_t x, uint8_t y, char ch);

#endif
