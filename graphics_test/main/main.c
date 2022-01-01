
// System.
#include <stdio.h>
#include <esp_system.h>
#include <sdkconfig.h>
#include <esp_err.h>
#include <esp_log.h>

// Screen.
#include <driver/spi_master.h>
#include <hal/spi_types.h>
#include "ili9341.h"

// Configuration.
#define SPI_MOSI 12
#define SPI_MISO 13
#define SPI_SCLK 21
#define SPI_MAX_TRANSFER 4094
#define SPI_BUS 2 //VSPI_HOST
#define SPI_DMA_CH 2

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
	display.pin_reset  = -1;
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
	
	// Send a test pattern.
	
	// There's nothing to do, really.
	
}
