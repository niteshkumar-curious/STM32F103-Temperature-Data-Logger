#ifndef I2C_PDRIVER_H_
#define I2C_PDRIVER_H_

#include <stdint.h>

void I2C_Init(void);
uint8_t I2C_WriteCommand(const uint8_t *command, uint16_t length);
uint8_t I2C_WriteData(const uint8_t *data, uint16_t length);

#endif