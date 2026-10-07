#ifndef DATA_LOGGER_H
#define DATA_LOGGER_H

#include <stdint.h>
#include <stdbool.h>

#define PAGE_SIZE          256UL
#define LOGS_PER_PAGE      (PAGE_SIZE / sizeof(uint32_t)) /* 64 samples per page */

#define TIME_BLOCK_START   0x000000UL
#define TIME_BLOCK_END     0x00FFFFUL

#define TEMP_BLOCK_START   0x010000UL
#define TEMP_BLOCK_END     0x02FFFFUL

extern uint32_t time_flash_addr;
extern uint32_t temp_flash_addr;

void logger_init(void);
void logger_add_reading(uint32_t timestamp, float temperature);
void logger_process_flash_task(void);

#endif