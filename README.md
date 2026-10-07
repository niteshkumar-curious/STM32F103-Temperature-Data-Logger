# STM32F103 Bare-Metal Temperature Data Logger

A bare-metal C temperature data logger based on the STM32F103C8TX.
The system measures temperature using an NTC thermistor, displays
real-time temperature on an SSD1306 OLED, and stores temperature
records in external W25Q128 SPI Flash memory.

## Features

- Bare-metal C firmware with direct peripheral-register access
- STM32F103C8TX Cortex-M3
- NTC thermistor temperature measurement using ADC
- ADC filtering using Exponential Moving Average (EMA)
- TIM3-based 1-second system time base
- Ping-pong/double buffering for temperature logging
- W25Q128 external SPI Flash storage
- SPI communication at approximately 1 MHz
- SSD1306 128x32 OLED display over I2C
- I2C communication at approximately 375 kHz
- 4x4 matrix keypad interface
- Temperature status indication using LEDs
- Metadata-based Flash storage management


## Project Structure

```text
STM32F103-Temperature-Data-Logger/
├── Documentation/
│   └── STM32_Temperature_Data_Logger_SRS.pdf
├── Inc/
│   ├── I2C_PDRIVER.h
│   ├── SSD1306_DIS.h
│   ├── Timer_1s.h
│   ├── data_logger.h
│   ├── keypad.h
│   ├── metadata.h
│   ├── spi_driver.h
│   ├── temp_sensor.h
│   └── w25q128.h
├── Src/
│   ├── I2C_PDRIVER.c
│   ├── SSD1306_DIS.c
│   ├── Timer_1s.c
│   ├── data_logger.c
│   ├── keypad.c
│   ├── main.c
│   ├── metadata.c
│   ├── spi_driver.c
│   ├── syscalls.c
│   ├── sysmem.c
│   ├── temp_sensor.c
│   └── w25q128.c
├── Startup/
│   └── startup_stm32f103c8tx.s
├── STM32F103C8TX_FLASH.ld
├── .gitignore
└── README.md
```