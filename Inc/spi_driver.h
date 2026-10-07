#ifndef SPI_DRIVER_H_
#define SPI_DRIVER_H_
#include <stdint.h>



typedef struct
{
	volatile uint8_t *tx_buffer;
	volatile uint8_t *rx_buffer;
	
	volatile uint16_t tx_index;
	volatile uint16_t rx_index;
	volatile uint16_t length;
	
	volatile uint8_t transfer_complete;
	
	
}spi_handle_t;

extern spi_handle_t spi1;

void spi_init(void);
void spi_transfer(uint8_t *tx, uint8_t *rx, uint16_t len);
uint8_t spi_transfer_complete(void);

#endif