#include "metadata.h"
#include "w25q128.h"
#include "Timer_1s.h"
#include "data_logger.h"

logger_metadata_t metadata;

/* Calculates XOR checksum over first 7 uint32_t fields */
static uint32_t metadata_checksum(const logger_metadata_t *m)
{
    const uint32_t *p = (const uint32_t *)m;
    uint32_t checksum = 0;

    for(uint8_t i = 0; i < 7; i++) {
        checksum ^= p[i];
    }

    return checksum;
}

/* Validates record magic and checksum */
static bool metadata_is_valid(const logger_metadata_t *m)
{
    if (m->magic != LOGGER_MAGIC) {
        return false;
    }

    if (m->checksum != metadata_checksum(m)) {
        return false;
    }

    return true;
}  

/* Finds the newest valid metadata record across both sectors */
bool logger_load_metadata(void)
{
    logger_metadata_t record;
    bool found = false;
    uint32_t newest_sequence = 0;

    for(uint32_t sector = 0; sector < 2; sector++)
    {
        uint32_t sector_start = (sector == 0) ? METADATA_SECTOR_A : METADATA_SECTOR_B;

        for(uint32_t i = 0; i < RECORDS_PER_SECTOR; i++)
        {
            uint32_t address = sector_start + (i * METADATA_RECORD_SIZE);

            w25q_read_data(address, (uint8_t *)&record, sizeof(record));

            if(!metadata_is_valid(&record)) {
                continue;
            }

            if(!found || record.sequence > newest_sequence) {
                metadata = record;
                newest_sequence = record.sequence;
                found = true;
            }
        }
    }

    return found;
}

/* 
 * Finds the next available record slot.
 * Automatically erases the alternate sector when a sector becomes full.
 */
static bool metadata_find_nxt_addr(uint32_t *address)
{
    logger_metadata_t record;

    uint32_t latest_address = 0;
    uint32_t latest_sequence = 0;
    bool found = false;

    for(uint32_t sector = 0; sector < 2; sector++)
    {
        uint32_t sector_start = (sector == 0) ? METADATA_SECTOR_A : METADATA_SECTOR_B;

        for(uint32_t i = 0; i < RECORDS_PER_SECTOR; i++)
        {
            uint32_t addr = sector_start + (i * METADATA_RECORD_SIZE);

            w25q_read_data(addr, (uint8_t *)&record, sizeof(record));

            if(!metadata_is_valid(&record)) {
                continue;
            }

            if(!found || record.sequence > latest_sequence) {
                latest_sequence = record.sequence;
                latest_address = addr;
                found = true;
            }
        }
    }

    /* First time setup: default to Sector A */
    if(!found) {
        w25q_sector_erase(METADATA_SECTOR_A);
        *address = METADATA_SECTOR_A;
        return true;
    }

    uint32_t next = latest_address + METADATA_RECORD_SIZE;

    /* Within Sector A */
    if(latest_address >= METADATA_SECTOR_A && latest_address < (METADATA_SECTOR_A + SECTOR_SIZE))
    {
        if(next < (METADATA_SECTOR_A + SECTOR_SIZE)) {
            *address = next;
        } else {
            /* Sector A full -> Erase Sector B and switch to Sector B */
            w25q_sector_erase(METADATA_SECTOR_B);
            *address = METADATA_SECTOR_B;
        }
        return true;
    }

    /* Within Sector B */
    if(latest_address >= METADATA_SECTOR_B && latest_address < (METADATA_SECTOR_B + SECTOR_SIZE))
    {
        if(next < (METADATA_SECTOR_B + SECTOR_SIZE)) {
            *address = next;
        } else {
            /* Sector B full -> Erase Sector A and wrap back to Sector A */
            w25q_sector_erase(METADATA_SECTOR_A);
            *address = METADATA_SECTOR_A;
        }
        return true;
    }

    return false;
}  

/* Readback verification */
static bool metadata_verify(uint32_t address, logger_metadata_t *expected)
{
    logger_metadata_t readback;

    w25q_read_data(address, (uint8_t *)&readback, sizeof(readback));

    if(!metadata_is_valid(&readback)) {
        return false;
    }

    if(readback.sequence != expected->sequence) {
        return false;
    }

    if(readback.time_flash_addr != expected->time_flash_addr) {
        return false;
    }

    if(readback.temp_flash_addr != expected->temp_flash_addr) {
        return false;
    }

    return true;
}  

/* Saves current state into next available slot in active metadata sector */
bool logger_save_metadata(void)
{
    logger_metadata_t new_metadata;
    uint32_t address;

    if(!metadata_find_nxt_addr(&address)) {
        return false;
    }

    new_metadata.timer_seconds   = timer_seconds;
    new_metadata.time_flash_addr = time_flash_addr;
    new_metadata.temp_flash_addr = temp_flash_addr;
    new_metadata.log_count       = metadata.log_count + LOGS_PER_PAGE;
    new_metadata.sequence        = metadata.sequence + 1;
    new_metadata.reserved        = 0;
    new_metadata.magic           = LOGGER_MAGIC;
    new_metadata.checksum        = metadata_checksum(&new_metadata);

    w25q_write_page(address, (uint8_t *)&new_metadata, sizeof(new_metadata));

    if(!metadata_verify(address, &new_metadata)) {
        return false;
    }

    metadata = new_metadata;
    return true;
}