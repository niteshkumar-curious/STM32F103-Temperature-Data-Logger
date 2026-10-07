#ifndef METADATA_H
#define METADATA_H

#include <stdint.h>
#include <stdbool.h>

#define LOGGER_MAGIC        0x4C4F4747UL

#define METADATA_SECTOR_A   0x030000UL
#define METADATA_SECTOR_B   0x031000UL
#define SECTOR_SIZE         4096UL

typedef struct 
{
    uint32_t magic;
    uint32_t sequence;

    uint32_t timer_seconds;
    uint32_t time_flash_addr;
    uint32_t temp_flash_addr;

    uint32_t log_count;
    uint32_t reserved;
    uint32_t checksum;

} logger_metadata_t;

#define METADATA_RECORD_SIZE sizeof(logger_metadata_t)
#define RECORDS_PER_SECTOR   (SECTOR_SIZE / METADATA_RECORD_SIZE)

extern logger_metadata_t metadata;

bool logger_load_metadata(void);
bool logger_save_metadata(void);

#endif
