
#ifndef MAIN_H
#define MAIN_H

#define GPIO_I2S_MCLK    0
#define GPIO_UART_TX     1
#define GPIO_SD_D0       2
#define GPIO_UART_RX     3
#define GPIO_I2S_LR      4
#define GPIO_LED_DATA    5
#define GPIO_I2S_CLK     12
#define GPIO_I2S_DATA    13
#define GPIO_SD_CLK      14
#define GPIO_SD_CMD      15
#define GPIO_SPI_CLK     18
#define GPIO_SD_PWR      19  // Also LED power
#define GPIO_I2C_SCL     21
#define GPIO_I2C_SDA     22
#define GPIO_SPI_MOSI    23
#define GPIO_LCD_RESET   25
#define GPIO_LCD_MODE    26
#define GPIO_SPI_CS_FPGA 27
#define GPIO_SPI_CS_LCD  32
#define GPIO_SPI_DC_LCD  33
#define GPIO_INT_RP2040  34  // Active low
#define GPIO_SPI_MISO    35
#define GPIO_INT_BNO055  36  // Active low
#define GPIO_INT_FPGA    39  // Active low

// I2C bus
#define I2C_BUS        0
#define I2C_SPEED      400000  // 400 kHz
#define I2C_TIMEOUT    250 // us

#define RP2040_ADDR 0x17  // RP2040 co-processor
#define BNO055_ADDR 0x28  // BNO055 position sensor
#define BME680_ADDR 0x77  // BME680 environmental sensor

// SPI bus
#define SPI_BUS               VSPI_HOST
#define SPI_MAX_TRANSFER_SIZE 4094
#define SPI_DMA_CHANNEL       2

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
#include <freertos/semphr.h>
#include <driver/uart.h>
#include <driver/i2c.h>
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

#include <rp2040.h>

#define MASK_BTN_START     (1 << RP2040_INPUT_BUTTON_START)
#define MASK_BTN_SELECT    (1 << RP2040_INPUT_BUTTON_SELECT)
#define MASK_BTN_MENU      (1 << RP2040_INPUT_BUTTON_MENU)
#define MASK_BTN_HOME      (1 << RP2040_INPUT_BUTTON_HOME)
#define MASK_BTN_JOY_LEFT  (1 << RP2040_INPUT_JOYSTICK_LEFT)
#define MASK_BTN_JOY_PRESS (1 << RP2040_INPUT_JOYSTICK_PRESS)
#define MASK_BTN_JOY_DOWN  (1 << RP2040_INPUT_JOYSTICK_DOWN)
#define MASK_BTN_JOY_UP    (1 << RP2040_INPUT_JOYSTICK_UP)
#define MASK_BTN_JOY_RIGHT (1 << RP2040_INPUT_JOYSTICK_RIGHT)
#define MASK_BTN_BACK      (1 << RP2040_INPUT_BUTTON_BACK)
#define MASK_BTN_ACCEPT    (1 << RP2040_INPUT_BUTTON_ACCEPT)

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
#include "pax_shapes.h"
#include "pax_codecs.h"

#ifdef __cplusplus
extern "C" {
#endif

extern xSemaphoreHandle i2c_semaphore;
extern uint16_t button_bits;

// Variables.
extern PCA9555   dev_pca9555;
extern ICE40     dev_ice40;
extern ILI9341   display;
extern RP2040    dev_rp2040;

extern uint8_t   framebuffer[ILI9341_BUFFER_SIZE];
extern pax_buf_t buf;
extern pax_buf_t clip;

#ifdef __cplusplus
}
#endif

#endif //MAIN_H
