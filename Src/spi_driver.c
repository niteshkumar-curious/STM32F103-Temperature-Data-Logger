#include "spi_driver.h"

#define RCC_APB2ENR 	(*(volatile uint32_t *)0x40021018)
#define GPIOA_CRL 		(*(volatile uint32_t *)0x40010800)
#define GPIOA_ODR 		(*(volatile uint32_t *)0x4001080C)
#define SPI_CR1 		(*(volatile uint16_t *)0x40013000)
#define SPI_CR2			(*(volatile uint16_t *)0x40013004)
#define SPI_SR 			(*(volatile uint16_t *)0x40013008)
#define SPI_DR 			(*(volatile uint16_t *)0x4001300C)
#define NVIC_ISER1 		(*(volatile uint32_t *)0xE000E104)
#define SCK 			(0xB << 20)
#define NSS 			(0x3 << 16)
#define MISO 			(0x4 << 24)
#define MOSI 			(0xB << 28)
#define SPI1_SR_RXNE 	(1U << 0)
#define SPI1_SR_TXE 	(1U << 1)
#define SPI1_SR_BSY 	(1U << 7)
#define SPI1_SR_OVR 	(1U << 6)
#define SPI1_CR2_TXEIE  (1U << 7)
#define SPI1_CR2_RXNEIE (1U << 6)        


spi_handle_t spi1;

void spi_init(void)
{
	RCC_APB2ENR |= (1 << 0) | (1 << 2) | (1 << 12); // AFIO, GPIOA,SPI1

	GPIOA_CRL &= (~(0xF << 16)) & (~(0xF << 20)) & (~(0xF << 24)) & (~(0xF << 28)); // SPI1- PA4 PA5 PA6 PA7 - RESET

	// SPI1- PA4 PA5 PA6 PA7 - ENABLE MODE: 50MHZ O/P, CONF: 10- O/P PUSH-PULL //
	GPIOA_CRL |= NSS | SCK | MISO | MOSI;

	 // CPOL-0 CPHA-0,MSB-7, 8BIT DFF, rxonly=0, bidmode=0
	SPI_CR1 &= (~(1 << 1)) & (~(1 << 0)) & (~(1 << 7)) & (~(1 << 11)) & (~(1 << 10)) & (~(1 << 15));

	// ,MSTR-1,BRR-010(fpclk/64), ssm=1,ssi=1
	SPI_CR1 |= (1 << 2) | (0x2 << 3) | (1 << 8) | (1 << 9);
    SPI_CR1 |= (1 << 6); // SPE-ENABLE

	SPI_CR2 &= ~SPI1_CR2_RXNEIE; //  RXNEIE
	SPI_CR2 &= ~SPI1_CR2_TXEIE; // TXEIE

	GPIOA_ODR |= (1 << 4);

	NVIC_ISER1 |= (1 << 3);

    

	
}



void SPI1_IRQHandler(void){

    if(SPI_SR & SPI1_SR_RXNE )
    {
        uint8_t received_byte = SPI_DR;

        if(spi1.rx_buffer !=0){
            spi1.rx_buffer[spi1.rx_index]= received_byte;
        }
        spi1.rx_index++;

        // transmit the next byte 
        if(spi1.tx_index < spi1.length){
            if(spi1.tx_buffer !=0){
                SPI_DR = spi1.tx_buffer[spi1.tx_index++];
            }
            else{
                SPI_DR = 0XFF;  
            }
        }

        else if(spi1.rx_index >= spi1.length){

            SPI_CR2 &= ~SPI1_CR2_RXNEIE;

            while (SPI_SR & SPI1_SR_BSY);

            GPIOA_ODR |= (1 << 4);
    
            spi1.transfer_complete = 1;
        }
    }

    if(SPI_SR & SPI1_SR_OVR){
        volatile uint32_t temp= SPI_DR;

        temp = SPI_SR;
        (void)temp;
    }
}


void spi_transfer(uint8_t *tx, uint8_t *rx, uint16_t len)
{
		spi1.tx_buffer = tx;

		spi1.rx_buffer = rx;

		spi1.transfer_complete = 0;

		spi1.length = len;

		spi1.tx_index = 0;

		spi1.rx_index = 0;

    uint8_t dummy = SPI_DR;
    (void)dummy;

    GPIOA_ODR &= (~(1<<4));
    
    SPI_CR2 |= SPI1_CR2_RXNEIE; // Enable RXNE interrupt

    // SEND FIRST BYTE TO START CLK SIGN
    if(spi1.tx_buffer !=0){
        SPI_DR = spi1.tx_buffer[spi1.tx_index++];
    }
    else{
        SPI_DR = 0XFF;
    }

	
}


uint8_t spi_transfer_complete(void)
{
    return spi1.transfer_complete;
}
