#include "keypad.h"
#include "w25q128.h"
#include "data_logger.h"
#include "SSD1306_DIS.h"


#define RCC_APB2ENR       (*(volatile uint32_t*)0x40021018)
#define GPIOB_CRL        (*(volatile uint32_t*)0x40010C00)
#define GPIOB_CRH        (*(volatile uint32_t*)0x40010C04)
#define GPIOB_ODR        (*(volatile uint32_t*)0x40010C0C)
#define GPIOB_IDR        (*(volatile uint32_t*)0x40010C08)

#define GPIO_B_R1        (0x8 << 0)
#define GPIO_B_R2        (0x8 << 4)
#define GPIO_B_R3        (0x8 << 16)
#define GPIO_B_R4        (0x8 << 20)
#define GPIO_B_C1        (0x3 << 24)
#define GPIO_B_C2        (0x3 << 28)
#define GPIO_B_C3        (0x3 << 0)
#define GPIO_B_C4        (0x3 << 4)


#define GPIO_B_LED_G     (0x1U << 16)  
#define GPIO_B_LED_R     (0x1U << 20)  
#define GPIO_B_LED_B     (0x1U << 24)  


#define R1               0U
#define R2               1U
#define R3               4U
#define R4               5U
#define C1               6U
#define C2               7U
#define C3               8U
#define C4               9U

/* Alert LED Pin Definitions */
#define LED_GREEN_PIN    12U 
#define LED_RED_PIN      13U 
#define LED_BLUE_PIN     14U 

static const uint32_t rows[4] = {R1, R2, R3, R4};
static const uint32_t cols[4] = {C1, C2, C3, C4};

static const char keypad_map[16] = {
    '1', '2', '3', 'A',
    '4', '5', '6', 'B',
    '7', '8', '9', 'C',
    '*', '0', '#', 'D'
};

static bool key_released = true;

static void gpio_set_level(uint32_t pinb_no, uint8_t level)
{
    if (level == 1)
        GPIOB_ODR |= (1U << pinb_no);
    else
        GPIOB_ODR &= ~(1U << pinb_no);
}

static uint32_t gpio_get_level(uint32_t pin_no)
{
    return (GPIOB_IDR >> pin_no) & 1U;
}

void keypad_gpio_init(void)
{
    RCC_APB2ENR |= (1U << 3); // Enable GPIOB
    GPIOB_CRH &= ~(0xF << 0) & ~(0xF << 4) & ~(0xF << 16) & ~(0xF << 20) & ~(0xF << 24);
    GPIOB_CRL &= ~(0xF << 0) & ~(0xF << 4) & ~(0xF << 16) & ~(0xF << 20) & ~(0xF << 24) & ~(0xF << 28);

    GPIOB_CRH |= GPIO_B_C3 | GPIO_B_C4 | GPIO_B_LED_G | GPIO_B_LED_R | GPIO_B_LED_B;
    GPIOB_CRL |= GPIO_B_R2 | GPIO_B_R3 | GPIO_B_R4 | GPIO_B_C1 | GPIO_B_C2 | GPIO_B_R1;

    for (uint8_t i = 0; i < 4; i++)
    {
        gpio_set_level(cols[i], 1);
        gpio_set_level(rows[i], 1);
    }
    gpio_set_level(LED_GREEN_PIN, 0);
    gpio_set_level(LED_RED_PIN, 0);
    gpio_set_level(LED_BLUE_PIN, 0);
}

static char keypad_scan(void)
{
    for (int i = 0; i < 4; i++)
    {
        for (int k = 0; k < 4; k++)
        {
            gpio_set_level(cols[k], 1);
        }

        gpio_set_level(cols[i], 0);

        for (int j = 0; j < 4; j++)
        {
            if (gpio_get_level(rows[j]) == 0)
            {
                uint8_t index = (uint8_t)(j * 4 + i);
                return keypad_map[index];
            }
        }
    }
    return '\0';
}

char keypad_get_key(void)
{
    char key = keypad_scan();

    if (key == '\0')
    {
        key_released = true;
        return '\0';
    }

    if (!key_released)
    {
        return '\0';
    }

    if (keypad_scan() == key)
    {
        key_released = false;
        return key;
    }

    return '\0';
}


temp_sample_t sample_buffer[64];
uint8_t current_view_page = 0; // Pages 0, 1, 2 (16 samples per page)
char last_key_pressed = '\0';
bool data_loaded = false;


void load_flash_samples(uint32_t start_time_addr, uint32_t start_temp_addr)
{
    for (uint16_t i = 0; i < 64; i++)
    {
        uint32_t t_val = 0;
        float f_val = 0.0f;

        w25q_read_data(start_time_addr + (i * sizeof(uint32_t)), (uint8_t *)&t_val, sizeof(uint32_t));
        w25q_read_data(start_temp_addr + (i * sizeof(float)), (uint8_t *)&f_val, sizeof(float));

        sample_buffer[i].time_sec = t_val;
        sample_buffer[i].temp_c = f_val;
    }
    data_loaded = true;
}


void render_flash_data_page(uint8_t page_num)
{
    if (!data_loaded)
    {
        OLED_Display_clear();
        OLED_Display_String(0, 0, "No Flash Data Found");
        SSD1306_updateScreen();
        return;
    }

    uint16_t start_idx = page_num * 16;
    float temp_sum = 0.0f;
    uint32_t time_start = sample_buffer[start_idx].time_sec;
    uint32_t time_end = sample_buffer[start_idx + 15].time_sec;

    for (uint16_t i = start_idx; i < start_idx + 16; i++)
    {
        temp_sum += sample_buffer[i].temp_c;
    }
    float avg_temp = temp_sum / 16.0f;

    OLED_Display_clear();

    // Page 0/Row 0: Header Info
    OLED_Display_String(0, 0, "Pg:");
    OLED_Display_Number(18, 0, page_num + 1);
    OLED_Display_String(30, 0, " T:");
    OLED_Display_Number(48, 0, time_start);
    OLED_Display_String(72, 0, "-");
    OLED_Display_Number(78, 0, time_end);
    OLED_Display_String(108, 0, "s");

    // Page 1/Row 8: Average Temperature
    OLED_Display_String(0, 8, "Avg Temp:");
    OLED_Display_Float(60, 8, avg_temp);

    // Page 2/Row 16: Navigation guidance
    OLED_Display_String(0, 16, "D:Nxt B:Prev *:Exit");

    // Page 3/Row 24: Input Status display
    OLED_Display_String(0, 24, "Input Status:");
    if (last_key_pressed != '\0')
    {
        OLED_Display_Char(84, 24, last_key_pressed);
    }

    SSD1306_updateScreen();
}

void render_menu_prompt(void)
{
    OLED_Display_clear();
    OLED_Display_String(0, 0,  "*:Realtime Temp Mode");
    OLED_Display_String(0, 8,  "1:Read Recent Flash");
    OLED_Display_String(0, 16, "2:Read Prev Flash");
    OLED_Display_String(0, 24, "3:Read Old Flash");
    
    SSD1306_updateScreen();
}

void led_alert(float *temp_c)
{
    if (*temp_c > 40.0f)
    {
        /* High Temp Alert: RED ON, GREEN/BLUE OFF */
        gpio_set_level(LED_RED_PIN, 1);
        gpio_set_level(LED_GREEN_PIN, 0);
        gpio_set_level(LED_BLUE_PIN, 0);
    }
    else if (*temp_c < 20.0f)
    {
        /* Low Temp Alert: BLUE ON, RED/GREEN OFF */
        gpio_set_level(LED_BLUE_PIN, 1);
        gpio_set_level(LED_RED_PIN, 0);
        gpio_set_level(LED_GREEN_PIN, 0);
    }
    else
    {
        /* Normal Temperature: GREEN ON, RED/BLUE OFF */
        gpio_set_level(LED_GREEN_PIN, 1);
        gpio_set_level(LED_RED_PIN, 0);
        gpio_set_level(LED_BLUE_PIN, 0);
    }
}