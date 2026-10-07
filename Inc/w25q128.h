#ifndef W25Q128_H_
#define W25Q128_H_
#include <stdint.h>
#include <stdbool.h>

#define W25Q_MAX_PAGE_SIZE          256U

bool w25q_init(void);
void w25q_sector_erase(uint32_t address);
void w25q_block64_erase(uint32_t address);
void w25q_block32_erase(uint32_t address);
void w25q_read_data(uint32_t address,
               uint8_t *buffer,
               uint16_t length);

void w25q_write_page(uint32_t address,
											uint8_t *buffer, 
											uint16_t length);

#endif