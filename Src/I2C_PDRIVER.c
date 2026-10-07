#include "I2C_PDRIVER.h"


/*
 * I2C2 TIMING CALCULATION
 *
 * PCLK1 = 8 MHz
 * TPCLK1 = 1 / 8 MHz = 125 ns
 *
 * FREQ = 8
 *
 * Fast Mode:
 * FS = 1
 *
 * DUTY = 0
 *
 * For DUTY = 0:
 * fSCL = PCLK1 / (3 × CCR)
 *
 * CCR = 7
 *
 * fSCL = 8 MHz / (3 × 7)
 *      = 380.95 kHz
 *
 * Therefore measured SCL is approximately
 * 375–381 kHz depending on actual bus timing.
 *
 * TRISE:
 *
 * Fast Mode maximum rise time = 300 ns
 *
 * TRISE = (300 ns / 125 ns) + 1
 *       = 2.4 + 1
 *       = 3.4
 *
 * Integer part = 3
 *
 * Therefore:
 *
 * FREQ  = 8
 * DUTY  = 0
 * CCR   = 7
 * TRISE = 3
 */


#define RCC_APB2ENR     (*(volatile uint32_t *)0x40021018)
#define RCC_APB1ENR     (*(volatile uint32_t *)0x4002101C)

#define GPIOB_CRH       (*(volatile uint32_t *)0x40010C04)

#define I2C2_CR1        (*(volatile uint32_t *)0x40005800)
#define I2C2_CR2        (*(volatile uint32_t *)0x40005804)
#define I2C2_DR         (*(volatile uint32_t *)0x40005810)
#define I2C2_SR1        (*(volatile uint32_t *)0x40005814)
#define I2C2_SR2        (*(volatile uint32_t *)0x40005818)
#define I2C2_CCR        (*(volatile uint32_t *)0x4000581C)
#define I2C2_TRISE      (*(volatile uint32_t *)0x40005820)


#define APB1_I2C2       (1U << 22)
#define APB2_AFIO       (1U << 0)
#define APB2_PORT_B     (1U << 3)


/* PB10 = SCL, PB11 = SDA */
#define I2C2_SCL        (0xFU << 8)
#define I2C2_SDA        (0xFU << 12)


/* I2C timing */
#define I2C2_FREQ           8U
#define I2C2_CCR_VALUE      7U
#define I2C2_TRISE_VALUE    3U

#define I2C_CCR_FS          (1U << 15)
#define I2C_CCR_DUTY        (1U << 14)


/* CR1 */
#define I2C2_PE         (1U << 0)
#define CR1_START       (1U << 8)
#define CR1_STOP        (1U << 9)
#define CR1_ACK         (1U << 10)


/* SR1 */
#define SR1_SB          (1U << 0)
#define SR1_ADDR        (1U << 1)
#define SR1_BTF         (1U << 2)
#define SR1_TXE         (1U << 7)
#define SR1_AF          (1U << 10)


/* SR2 */
#define SR2_MSL         (1U << 0)
#define SR2_BSY         (1U << 1)
#define SR2_TRA         (1U << 2)


#define SSD1306_ADDR    0x3C


void I2C_Init(void)
{
    /* Enable I2C2 and GPIOB clocks */
    RCC_APB1ENR |= APB1_I2C2;
    RCC_APB2ENR |= APB2_AFIO | APB2_PORT_B;


    /* PB10 and PB11: Alternate Function Open Drain, 50 MHz */
    GPIOB_CRH &= ~(I2C2_SCL | I2C2_SDA);
    GPIOB_CRH |=  (I2C2_SCL | I2C2_SDA);


    /* Disable I2C before configuring timing */
    I2C2_CR1 &= ~I2C2_PE;


    /* PCLK1 = 8 MHz */
    I2C2_CR2 = I2C2_FREQ;


    /* Fast Mode, DUTY = 0, CCR = 7 */
    I2C2_CCR = I2C_CCR_FS | I2C2_CCR_VALUE;


    /* Fast Mode rise time */
    I2C2_TRISE = I2C2_TRISE_VALUE;


    /* Enable I2C */
    I2C2_CR1 |= I2C2_PE;
}


static uint8_t I2C_Start(void)
{
    volatile uint32_t temp;


    /* Wait until bus is free */
    while (I2C2_SR2 & SR2_BSY);


    /* Generate START */
    I2C2_CR1 |= CR1_START;


    /* Wait for START condition */
    while (!(I2C2_SR1 & SR1_SB));


    /* Send SSD1306 address + WRITE */
    I2C2_DR = (SSD1306_ADDR << 1);


    /* Wait for address ACK or failure */
    while (!(I2C2_SR1 & (SR1_ADDR | SR1_AF)));


    if (I2C2_SR1 & SR1_AF)
    {
        I2C2_SR1 &= ~SR1_AF;
        I2C2_CR1 |= CR1_STOP;

        return 1;
    }


    /* Clear ADDR flag */
    temp = I2C2_SR1;
    temp = I2C2_SR2;

    (void)temp;


    return 0;
}


uint8_t I2C_WriteCommand(const uint8_t *command,
                         uint16_t length)
{
    uint16_t i;


    if (I2C_Start() != 0)
    {
        return 1;
    }


    /* Wait until data register is empty */
    while (!(I2C2_SR1 & SR1_TXE));


    /* SSD1306 command control byte */
    I2C2_DR = 0x00;


    for (i = 0; i < length; i++)
    {
        /* Wait for TXE or ACK failure */
        while (!(I2C2_SR1 & (SR1_TXE | SR1_AF)));


        if (I2C2_SR1 & SR1_AF)
        {
            I2C2_SR1 &= ~SR1_AF;
            I2C2_CR1 |= CR1_STOP;

            return 2;
        }


        I2C2_DR = command[i];
    }


    /* Wait until final byte is transferred */
    while (!(I2C2_SR1 & (SR1_BTF | SR1_AF)));


    if (I2C2_SR1 & SR1_AF)
    {
        I2C2_SR1 &= ~SR1_AF;
        I2C2_CR1 |= CR1_STOP;

        return 3;
    }


    I2C2_CR1 |= CR1_STOP;


    return 0;
}


uint8_t I2C_WriteData(const uint8_t *data,
                      uint16_t length)
{
    uint16_t i;


    if (I2C_Start() != 0)
    {
        return 1;
    }


    /* Wait until data register is empty */
    while (!(I2C2_SR1 & SR1_TXE));


    /* SSD1306 data control byte */
    I2C2_DR = 0x40;


    for (i = 0; i < length; i++)
    {
        /* Wait for TXE or ACK failure */
        while (!(I2C2_SR1 & (SR1_TXE | SR1_AF)));


        if (I2C2_SR1 & SR1_AF)
        {
            I2C2_SR1 &= ~SR1_AF;
            I2C2_CR1 |= CR1_STOP;

            return 2;
        }


        I2C2_DR = data[i];
    }


    /* Wait until final byte is transferred */
    while (!(I2C2_SR1 & (SR1_BTF | SR1_AF)));


    if (I2C2_SR1 & SR1_AF)
    {
        I2C2_SR1 &= ~SR1_AF;
        I2C2_CR1 |= CR1_STOP;

        return 3;
    }


    I2C2_CR1 |= CR1_STOP;


    return 0;
}