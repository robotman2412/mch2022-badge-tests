
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

#include "../components/pax-graphics-lib/test-images/empty.c"

ILI9341 display;

uint8_t framebuffer[ILI9341_BUFFER_SIZE];

static const char *TAG = "main";

pax_col_t test_shader_callback(pax_col_t tint, int x, int y, float u, float v, void *args) {
	if (!args) {
		return (u < 0.5) ^ (v >= 0.5) ? 0xffff00ff : 0xff1f1f1f;
	}
	pax_buf_t *image = (pax_buf_t *) args;
	return pax_get_pixel(image, u*image->width, v*image->height);
}

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
	ESP_LOGI(TAG, "Loading test image.");
	pax_buf_t image;
	pax_buf_init(&image, image_empty, 25, 25, PAX_BUF_32_8888ARGB);
	
	// Send a test pattern.
	ESP_LOGI(TAG, "Creating framebuffer.");
	pax_buf_t buf;
	pax_buf_init(&buf, framebuffer, ILI9341_WIDTH, ILI9341_HEIGHT, PAX_BUF_16_565RGB);
	if (pax_last_error) {
		ESP_LOGE(TAG, "Framebuffer creation failed.");
		return;
	}
	pax_apply_2d(&buf, matrix_2d_translate(buf.width / 2.0, buf.height / 2.0));
	// pax_apply_2d(&buf, matrix_2d_scale(2, 2));
	
	pax_shader_t test_shader = {
		.callback = test_shader_callback,
		.callback_args = &image
	};
	// pax_debug(&buf);
	
	while (1) {
		uint64_t millis = esp_timer_get_time() / 1000;
		
		pax_background(&buf, 0xff000000);
		pax_col_t color0 =   0xffffffff & pax_col_hsv(millis * 255 / 8000, 255, 255);
		pax_col_t color1 =   0x7fffffff & pax_col_hsv(millis * 255 / 8000 + 127, 255, 255);
		
		// Single triangle demo.
		// uint32_t a = millis / 16;
		//pax_clip(&buf, 0, 0, buf.width-((a >> 2) % buf.width), buf.height-((a >> 1) % buf.height));
		//pax_simple_rect(&buf, 0xff000000, 0, 0, buf.width, buf.height);
		// pax_draw_tri(&buf, -1, 0, 0, a % buf.width, buf.height / 2.0, buf.width / 2.0, a % buf.height);
		
		// Shading demo.
		pax_push_2d(&buf);
		float a0 = millis / 3000.0 * M_PI;
		pax_apply_2d(&buf, matrix_2d_rotate(a0));
		pax_shade_rect(&buf, -1, &test_shader, NULL, -25, -25, 50, 50);
		pax_pop_2d(&buf);
		char *text = "This is, a TEXT.";
		pax_vec1_t size = pax_text_size(NULL, 18, text);
		pax_draw_text(&buf, -1, NULL, 18, -size.x / 2, buf.height / 2 - size.y, "This is, a TEXT.");
		
		// Epic arcs demo.
		// float a0 = millis / 3000.0 * M_PI;
		// float a1 = fmodf(a0, M_PI * 4) - M_PI * 2;
		// pax_draw_arc(&buf, color0, 0, 0, 1, a0, a0 + a1);
		// pax_push_2d(&buf);
		
		// pax_apply_2d(&buf, matrix_2d_rotate(a0));
		// pax_push_2d(&buf);
		// pax_apply_2d(&buf, matrix_2d_translate(1, 0));
		// pax_draw_rect(&buf, color1, -0.25, -0.25, 0.5, 0.5);
		// pax_pop_2d(&buf);
		
		// pax_apply_2d(&buf, matrix_2d_rotate(a1));
		// pax_push_2d(&buf);
		// pax_apply_2d(&buf, matrix_2d_translate(1, 0));
		// pax_apply_2d(&buf, matrix_2d_rotate(-a0 - a1 + M_PI * 0.5));
		// pax_draw_tri(&buf, color1, 0.25, 0, -0.125, 0.2165, -0.125, -0.2165);
		// pax_pop_2d(&buf);
		
		// pax_pop_2d(&buf);
		
		if (ili9341_write(&display, framebuffer)) {
			ESP_LOGE(TAG, "Display write failed.");
			vTaskDelay(3000 / portTICK_PERIOD_MS);
			esp_restart();
		}
		
		// if (millis > 1500) break;
	}
}
