#include "data_logger.h"
#include "metadata.h"
#include "w25q128.h"
#include "Timer_1s.h"

/* Global flash pointers */
uint32_t time_flash_addr = TIME_BLOCK_START;
uint32_t temp_flash_addr = TEMP_BLOCK_START;

/* Double ping-pong buffers */
static uint32_t time_buffer_A[LOGS_PER_PAGE];
static uint32_t time_buffer_B[LOGS_PER_PAGE];

static float temp_buffer_A[LOGS_PER_PAGE];
static float temp_buffer_B[LOGS_PER_PAGE];



static uint8_t  active_buffer = 0;
static uint16_t buffer_index  = 0;

static bool flag_write_A = false;
static bool flag_write_B = false;

/* =========================================================
   Logger Initialization & Power-Recovery Connection
   ========================================================= */
void logger_init(void)
{
    active_buffer = 0;
    buffer_index  = 0;

    flag_write_A = false;
    flag_write_B = false;

    /* Check if previous session metadata exists in SPI Flash */
    if(logger_load_metadata())
    {
        /* Restore operational addresses & state after power cycle */
        time_flash_addr = metadata.time_flash_addr;
        temp_flash_addr = metadata.temp_flash_addr;
        timer_seconds   = metadata.timer_seconds;
    }
    else
    {
        /* Fresh start / Uninitialized SPI Flash */
        time_flash_addr    = TIME_BLOCK_START;
        temp_flash_addr    = TEMP_BLOCK_START;
        timer_seconds      = 0;
        metadata.log_count = 0;
        metadata.sequence  = 0;
    }

    for(uint16_t i = 0; i < LOGS_PER_PAGE; i++)
    {
        time_buffer_A[i] = 0;
        time_buffer_B[i] = 0;

        temp_buffer_A[i] = 0.0f;
        temp_buffer_B[i] = 0.0f;
    }
}

void logger_add_reading(uint32_t timestamp, float temperature)
{
    if(active_buffer == 0)
    {
        time_buffer_A[buffer_index] = timestamp;
        temp_buffer_A[buffer_index] = temperature;
        buffer_index++;

        if(buffer_index >= LOGS_PER_PAGE)
        {
            flag_write_A  = true;
            active_buffer = 1;
            buffer_index  = 0;
        }
    }
    else
    {
        time_buffer_B[buffer_index] = timestamp;
        temp_buffer_B[buffer_index] = temperature;
        buffer_index++;

        if(buffer_index >= LOGS_PER_PAGE)
        {
            flag_write_B  = true;
            active_buffer = 0;
            buffer_index  = 0;
        }
    }
}

static void logger_write_page(void)
{
    uint8_t *time_data;
    uint8_t *temp_data;

    if(flag_write_A)
    {
        time_data = (uint8_t *)time_buffer_A;
        temp_data = (uint8_t *)temp_buffer_A;
    }
    else if(flag_write_B)
    {
        time_data = (uint8_t *)time_buffer_B;
        temp_data = (uint8_t *)temp_buffer_B;
    }
    else
    {
        return;
    }

    /* 1. Write Timestamp Page */
    if((time_flash_addr % SECTOR_SIZE) == 0)
    {
        w25q_sector_erase(time_flash_addr);
    }

    w25q_write_page(time_flash_addr, time_data, PAGE_SIZE);
    time_flash_addr += PAGE_SIZE;

    if(time_flash_addr > TIME_BLOCK_END)
    {
        time_flash_addr = TIME_BLOCK_START;
    }

    /* 2. Write Temperature Page */
    if((temp_flash_addr % SECTOR_SIZE) == 0)
    {
        w25q_sector_erase(temp_flash_addr);
    }

    w25q_write_page(temp_flash_addr, temp_data, PAGE_SIZE);
    temp_flash_addr += PAGE_SIZE;

    if(temp_flash_addr > TEMP_BLOCK_END)
    {
        temp_flash_addr = TEMP_BLOCK_START;
    }

    /* 3. Journal Metadata state to flash */
    logger_save_metadata();

    /* 4. Reset write pending flags */
    if(flag_write_A)
    {
        flag_write_A = false;
      
    }
    else
    {
        flag_write_B = false;
       
    }
}

void logger_process_flash_task(void)
{
    logger_write_page();
}