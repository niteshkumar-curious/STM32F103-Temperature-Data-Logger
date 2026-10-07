#include "w25q128.h"
#include "spi_driver.h"
#include <stddef.h>

#define W25Q_MANUFACTURER_ID        0xEF
#define W25Q_MEMORY_TYPE            0x70
#define W25Q_CAPACITY_ID            0x18
#define W25Q_CMD_READ_JEDEC_ID      0x9F
#define W25Q_CMD_READ_JEDEC_ID_len  3
#define W25Q_CMD_READ_STATUS1       0x05
#define W25Q_CMD_WRITE_ENABLE       0x06
#define W25Q_CMD_WRITE_DISABLE      0x04
#define W25Q_CMD_PAGE_PROGRAM       0x02
#define W25Q_CMD_READ_DATA          0x03
#define W25Q_CMD_SECTOR_ERASE       0x20
#define W25Q_BUSY_BIT               0x01
#define W25Q_CMD_BLOCK64_ERASE      0xD8
#define W25Q_CMD_BLOCK32_ERASE      0x52
#define W25Q_CMD_SIZE               1U
#define W25Q_ADDR_SIZE              3U
#define W25Q_TRANSFER_SIZE          (W25Q_CMD_SIZE + W25Q_ADDR_SIZE + W25Q_MAX_PAGE_SIZE)

static uint8_t w25q_tx_buffer[W25Q_TRANSFER_SIZE];
static uint8_t w25q_rx_buffer[W25Q_TRANSFER_SIZE];


static void w25q_transfer(const uint8_t *tx, uint8_t *rx, uint16_t len)
{

    uint8_t dummy_rx[4]; 
    uint8_t *safe_rx = rx;
    if (rx == NULL && len <= 4)
    {
        safe_rx = dummy_rx;
    }

    spi_transfer((uint8_t*)tx, safe_rx, len);

    while (!spi_transfer_complete())
    {
        // Wait for transfer to complete
    }
}

static void w25q_read_jedec_id(uint8_t *id)
{
    uint8_t tx[4] = {W25Q_CMD_READ_JEDEC_ID, 0xFF, 0xFF, 0xFF};
    uint8_t rx[4] = {0};
    
    w25q_transfer(tx, rx, 4);
    
    for(uint8_t i = 0; i < 3; i++)
    {
        id[i] = rx[i + 1];
    }
}

bool w25q_init(void)
{
    uint8_t id[W25Q_CMD_READ_JEDEC_ID_len] = {0};

    w25q_read_jedec_id(id);

    if(id[0] == W25Q_MANUFACTURER_ID &&
       id[1] == W25Q_MEMORY_TYPE &&
       id[2] == W25Q_CAPACITY_ID)
    {
        return true;/* Flash detected successfully */
    }
    else
    {
        return false; /* Flash not detected */
    }
}

static uint8_t w25q_read_status(void)
{
    uint8_t tx[2] = {W25Q_CMD_READ_STATUS1, 0xFF};
    uint8_t rx[2] = {0};
    
    w25q_transfer(tx, rx, 2);
    
    return rx[1];
}

static void w25q_write_enable(void)
{
    uint8_t tx[1] = {W25Q_CMD_WRITE_ENABLE};
    uint8_t rx[1];

    w25q_transfer(tx, rx, 1);
}



void w25q_sector_erase(uint32_t address)
{
    // 1. Enable writing before erasing
    w25q_write_enable();

    uint8_t tx[4];
    tx[0] = W25Q_CMD_SECTOR_ERASE;
    tx[1] = (address >> 16) & 0xFF;
    tx[2] = (address >> 8) & 0xFF;
    tx[3] = address & 0xFF;

    w25q_transfer(tx, NULL, 4);
    
    // Wait for the erase to finish
    while(w25q_read_status() & W25Q_BUSY_BIT)
    {
        // wait
    }
		
}


void w25q_block64_erase(uint32_t address)
{
    // 1. Enable writing before erasing
    w25q_write_enable();

    uint8_t tx[4];
    tx[0] = W25Q_CMD_BLOCK64_ERASE;
    tx[1] = (address >> 16) & 0xFF;
    tx[2] = (address >> 8) & 0xFF;
    tx[3] = address & 0xFF;

    w25q_transfer(tx, NULL, 4);
    
    // Wait for the erase to finish
    while(w25q_read_status() & W25Q_BUSY_BIT)
    {
        // wait
    }
		
}


void w25q_block32_erase(uint32_t address)
{
    // 1. Enable writing before erasing
    w25q_write_enable();

    uint8_t tx[4];
    tx[0] = W25Q_CMD_BLOCK32_ERASE;
    tx[1] = (address >> 16) & 0xFF;
    tx[2] = (address >> 8) & 0xFF;
    tx[3] = address & 0xFF;

    w25q_transfer(tx, NULL, 4);
    
    // Wait for the erase to finish
    while(w25q_read_status() & W25Q_BUSY_BIT)
    {
        // wait
    }
		
}


void w25q_write_page(uint32_t address,uint8_t *buffer, uint16_t length)
{
	
		if(length > W25Q_MAX_PAGE_SIZE)
		{
			 length = W25Q_MAX_PAGE_SIZE;
		}
		
    // 1. Enable writing before page program
    w25q_write_enable();

   
    w25q_tx_buffer[0] = W25Q_CMD_PAGE_PROGRAM;
    w25q_tx_buffer[1] = (address >> 16) & 0xFF;	
    w25q_tx_buffer[2] = (address >> 8) & 0xFF;
    w25q_tx_buffer[3] = address & 0xFF;
		
		for(uint16_t i=0;i<length;i++)
		{
			w25q_tx_buffer[4+i] = buffer[i];
		}

    w25q_transfer(w25q_tx_buffer, NULL, 4+length);
    
    // Wait for the page program to finish
    while(w25q_read_status() & W25Q_BUSY_BIT)
    {
        // wait
    }
		
}


void w25q_read_data(uint32_t address, uint8_t *buffer, uint16_t length)
{
    // Prevent buffer overflow 
    if (length > W25Q_MAX_PAGE_SIZE)
    {
        length = W25Q_MAX_PAGE_SIZE; 
    }

    w25q_tx_buffer[0] = W25Q_CMD_READ_DATA;
    w25q_tx_buffer[1] = (address >> 16) & 0xFF;
    w25q_tx_buffer[2] = (address >> 8) & 0xFF;
    w25q_tx_buffer[3] = address & 0xFF;
    
    uint16_t total = length + 4;
    
   
    for(uint16_t i = 4; i < total; i++)
    {
        w25q_tx_buffer[i] = 0xFF;
    }
    
    w25q_transfer(w25q_tx_buffer, w25q_rx_buffer, total);
    
    for(uint16_t i = 0; i < length; i++)
    {
        buffer[i] = w25q_rx_buffer[i + 4];
    }
}

