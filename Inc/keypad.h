#ifndef KEYPAD_H
#define KEYPAD_H

#include <stdint.h>
#include <stdbool.h>



typedef struct {
    uint32_t time_sec;
    float temp_c;
} temp_sample_t;

extern temp_sample_t sample_buffer[64];
extern uint8_t current_view_page; 
extern char last_key_pressed;
extern bool data_loaded;

void keypad_gpio_init(void);
char keypad_get_key(void);
void load_flash_samples(uint32_t start_time_addr, uint32_t start_temp_addr);
void render_flash_data_page(uint8_t page_num);
void render_menu_prompt(void);
void led_alert(float *temp_c);

#endif
