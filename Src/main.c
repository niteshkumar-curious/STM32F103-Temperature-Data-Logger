#include "spi_driver.h"
#include "w25q128.h"
#include "temp_sensor.h"
#include "data_logger.h"
#include "SSD1306_DIS.h"
#include "I2C_PDRIVER.h"
#include "Timer_1s.h"
#include "metadata.h"
#include "keypad.h"

#define PAGE_SIZE_BYTES (64 * sizeof(uint32_t))
typedef enum {
    SYS_MODE_TEMP_READING = 0,
    SYS_MODE_KEYPAD_MENU
} system_mode_t;

int main(void)
{
    spi_init();
    I2C_Init();
    SSD1306_Init();
    keypad_gpio_init();

    if (!w25q_init())
    {
        OLED_Display_clear();
        OLED_Display_String(0, 0, "    Flash Not ");
        OLED_Display_String(0, 8, "    Detected !!   ");
        SSD1306_updateScreen();
        while (1);
    }

    OLED_Display_clear();
    OLED_Display_String(0, 0, "    Flash Detected  ");
    OLED_Display_String(0, 8, "    Successfully !! ");
    SSD1306_updateScreen();

    temp_sensor_init();
    Timer3_init();
    logger_init();

    system_mode_t current_mode = SYS_MODE_TEMP_READING;
    float current_temperature = 0.0f;

    while (1)
    {
        char key = keypad_get_key();

        if (key != '\0')
        {
            last_key_pressed = key;

            /* Return to Realtime Reading Mode when '*' is pressed */
            if (key == '*')
            {
                current_mode = SYS_MODE_TEMP_READING;
                data_loaded = false;
            }
            /* Enter Flash History Menu Mode when '#' is pressed */
            else if (key == '#')
            {
                current_mode = SYS_MODE_KEYPAD_MENU;
                render_menu_prompt();
            }
        }

        if (current_mode == SYS_MODE_TEMP_READING)
        {
            if (temp_sensor_get(&current_temperature))
            {
                /* Update Alert LED Hardware state */
                led_alert(&current_temperature);

                OLED_Display_clear();

                /* Display output based on temperature ranges */
                if (current_temperature > 40.0f)
                {
                    OLED_Display_String(32, 0, "HIGH TEMP");
                    OLED_Display_Float(40, 12, current_temperature);
                    OLED_Display_String(0, 24, "    # : FOR MENU");

                }
                else if (current_temperature < 20.0f)
                {
                    OLED_Display_String(32, 0, "LOW TEMP");
                    OLED_Display_Float(40, 12, current_temperature);
                    OLED_Display_String(0, 24, "    # : FOR MENU");
                }
                else
                {
                    OLED_Display_String(30, 0, "Temperature");
                    OLED_Display_Float(40, 12, current_temperature);
                    OLED_Display_String(0, 24, "    # : FOR MENU");
                }

                /* Out-of-bounds threshold logging flag */
                if ((current_temperature > 40.0f) || (current_temperature < 20.0f))
                {
                    uint32_t event_time = timer_seconds;
                    logger_add_reading(event_time, current_temperature);
                }
                
                SSD1306_updateScreen();
            }
            
            logger_process_flash_task();
        }
        else if (current_mode == SYS_MODE_KEYPAD_MENU)
        {
            if (key != '\0')
            {
                if (key == '1' || key == '2' || key == '3' || key == '4' || key == '5')
                {
                    /* Read Historical Data based on selection offset */
                    if (logger_load_metadata())
                    {
                        uint8_t page_offset_multiplier = key - '0';
                        uint32_t byte_offset           = page_offset_multiplier * PAGE_SIZE_BYTES;
                        uint32_t recent_t_addr = (metadata.time_flash_addr >= byte_offset) ?
                                                 (metadata.time_flash_addr - byte_offset) : 0;
                        uint32_t recent_temp_addr = (metadata.temp_flash_addr >= byte_offset) ?
                                                    (metadata.temp_flash_addr - byte_offset) : 0;

                        load_flash_samples(recent_t_addr, recent_temp_addr);
                        current_view_page = 0;
                        render_flash_data_page(current_view_page);
                    }
                    else
                    {
                        OLED_Display_clear();
                        OLED_Display_String(0, 0, "No Metadata Found");
                        SSD1306_updateScreen();
                    }
                }
                else if (key == 'D' || key == 'd')
                {
                    if (current_view_page < 2)
                    {
                        current_view_page++;
                    }
                    render_flash_data_page(current_view_page);
                }
                else if (key == 'B' || key == 'b')
                {
                    if (current_view_page > 0)
                    {
                        current_view_page--;
                    }
                    render_flash_data_page(current_view_page);
                }
            }
        }
    }
}


