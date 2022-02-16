
#ifndef MAIN_H
#define MAIN_H

// Prototype configuration.
#define SPI_SCLK         18
#define SPI_MOSI         23
#define SPI_MISO         35
#define SPI_CS_STM32     19
#define SPI_CS_FPGA      27
#define SPI_CS_LCD       32
#define SPI_DC_LCD       33
#define SPI_BUS          VSPI_HOST
#define SPI_MAX_TRANSFER 4094
#define SPI_DMA_CH       2

#define DISPLAY_RST     -1
#define DISPLAY_CS       SPI_CS_LCD
#define DISPLAY_DCX      SPI_DC_LCD

// Interrupts
#define GPIO_INT_STM32   0
#define GPIO_INT_PCA9555 34
#define GPIO_INT_BNO055  36
#define GPIO_INT_FPGA    39

// System I2C bus
#define GPIO_I2C_SYS_SCL 21
#define GPIO_I2C_SYS_SDA 22
#define I2C_BUS_SYS      0
#define I2C_SPEED_SYS    20000 // 20 kHz

// RP2040
#define RP2040_I2C_BUS   I2C_BUS_SYS
#define RP2040_I2C_ADDR  0x17
#define RP2040_I2C_IRQ   0
#define RP2040_REG_LCDM  4

// PCA9555 IO expander
#define PCA9555_ADDR              0x26
#define PCA9555_PIN_STM32_RESET   0
#define PCA9555_PIN_STM32_BOOT0   1
#define PCA9555_PIN_FPGA_RESET    2
#define PCA9555_PIN_FPGA_CDONE    3
#define PCA9555_PIN_BTN_START     5
#define PCA9555_PIN_BTN_SELECT    6
#define PCA9555_PIN_BTN_MENU      7
#define PCA9555_PIN_BTN_HOME      8
#define PCA9555_PIN_BTN_JOY_LEFT  9
#define PCA9555_PIN_BTN_JOY_PRESS 10
#define PCA9555_PIN_BTN_JOY_DOWN  11
#define PCA9555_PIN_BTN_JOY_UP    12
#define PCA9555_PIN_BTN_JOY_RIGHT 13
#define PCA9555_PIN_BTN_BACK      14
#define PCA9555_PIN_BTN_ACCEPT    15

// System.
#include <stdio.h>
#include <esp_system.h>
#include <sdkconfig.h>
#include <esp_err.h>
#include <esp_log.h>
#include <stdint.h>
#include <malloc.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/uart.h>
#include <esp_timer.h>
#include <driver/spi_master.h>
#include <driver/gpio.h>
#include <esp_pm.h>

#include <string.h>
#include <stdlib.h>

#include <driver/spi_master.h>
#include <hal/spi_types.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <managed_i2c.h>

// I/O expander.
#include <pca9555.h>

// FPGA.
#include <ice40.h>

// Screen.
#include "ili9341.h"

#ifdef __cplusplus
}
#endif

// GFX library.
#include "pax_gfx.h"
#include "pax_shaders.h"
#include "pax_codecs.h"
#include "fake_file.h"

#ifdef __cplusplus
extern "C" {
#endif

// Variables.
extern PCA9555   dev_pca9555;
extern ICE40     dev_ice40;
extern ILI9341   display;

extern uint8_t   framebuffer[ILI9341_BUFFER_SIZE];
extern pax_buf_t buf;
extern pax_buf_t clip;

#ifdef __cplusplus
}
#endif

#endif //MAIN_H
