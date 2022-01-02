
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
#include <esp_timer.h>

// Screen.
#include <driver/spi_master.h>
#include <hal/spi_types.h>
#include "ili9341.h"

// GFX library.
#include "pax_gfx.h"

// Configuration.
#define SPI_MOSI 12
#define SPI_MISO 13
#define SPI_SCLK 21
#define SPI_MAX_TRANSFER 4094
#define SPI_BUS ((spi_host_device_t) 2) //VSPI_HOST
#define SPI_DMA_CH 2

#define DISPLAY_RST 4
#define DISPLAY_CS 16
#define DISPLAY_DCX 17

ILI9341 display;

uint8_t *framebuffer;

static const char *TAG = "main";

void app_main() {
	
	// Let's see.
	printf("I booted!\n");
	
	// Initialise SPI bus.
	spi_bus_config_t spiConfig = {
		.mosi_io_num = SPI_MOSI,
		.miso_io_num = SPI_MISO,
		.sclk_io_num = SPI_SCLK,
		.quadwp_io_num = -1,
		.quadhd_io_num = -1,
		.max_transfer_sz = SPI_MAX_TRANSFER
	};
	if (spi_bus_initialize(SPI_BUS, &spiConfig, SPI_DMA_CH)) {
		ESP_LOGE(TAG, "SPI bus initialisation failed.");
		esp_restart();
	} else {
		ESP_LOGI(TAG, "SPI bus initialised.");
	}
	
	// Initialise display.
	display.spi_bus    = SPI_BUS;
	display.pin_cs     = DISPLAY_CS;
	display.pin_dcx    = DISPLAY_DCX;
	display.pin_reset  = DISPLAY_RST;
	display.rotation   = 1;
	display.color_mode = true;
	display.spi_speed  = 60000000; // 60 MHz
	display.spi_max_transfer_size = SPI_MAX_TRANSFER;
	display.callback   = NULL; // No callbacks just yet.
	if (ili9341_init(&display)) {
		ESP_LOGE(TAG, "Display initialisation failed.");
		esp_restart();
	} else {
		ESP_LOGI(TAG, "Display initialised.");
	}
	
	// Initialise GFX.
	framebuffer = (uint8_t *) malloc(ILI9341_BUFFER_SIZE);
	
	// Send a test pattern.
	pax_buf_t buf;
	pax_buf_init(&buf, framebuffer, ILI9341_WIDTH, ILI9341_HEIGHT, PAX_BUF_16_565RGB);
	while (1) {
		uint64_t millis = esp_timer_get_time() / 1000;
		pax_background(&buf, 0x000000);
		pax_col_t color = pax_col_hsv(millis * 255 / 8000, 255, 255);
		float a0 = millis / 3000.0 * M_PI;
		float a1 = fmodf(a0, M_PI * 4) - M_PI * 2;
		pax_simple_arc(&buf, color, buf.width >> 1, buf.height >> 1, 50, a0, a0 + a1);
		
		if (ili9341_write(&display, framebuffer)) {
			ESP_LOGE(TAG, "Display write failed.");
			vTaskDelay(3000 / portTICK_PERIOD_MS);
			esp_restart();
		}
	}
}
