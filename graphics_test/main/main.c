
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
#include "pax_shaders.h"
#include "pax_codecs.h"

// Menu.
#include "sample_menu.h"

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

#include "../components/pax-graphics/test-images/empty.c"

ILI9341 display;

// Pallette test, just 8bpp for nou.
uint8_t pallette_test_img[] = {
	0,1,0,1,0,
	0,0,2,0,0,
	1,0,0,0,1,
	0,1,1,1,0
};

pax_col_t pallette_test_pal[] = {
	pax_col_argb(0,   0,   0,   0),
	pax_col_argb(255, 255, 255, 255),
	pax_col_argb(255, 127, 127, 0)
};

uint8_t framebuffer[ILI9341_BUFFER_SIZE];

static const char *TAG = "main";

pax_col_t regenboogkots(pax_col_t tint, int x, int y, float u, float v, void *args) {
	return pax_col_hsv(x / 50.0 * 255.0 + y / 150.0 * 255.0, 255, 255);
}

void my_graphics_function() {
    // Setup.
    pax_buf_t buffer;
    pax_buf_init(&buffer, framebuffer, 320, 240, PAX_BUF_16_565RGB);
    
    // Green background.
    pax_background(&buffer, pax_col_rgb(0, 255, 0));
    
    // Red circle.
    float midpoint_x = buffer.width  / 2.0; // Middle of the screen horizontally.
    float midpoint_y = buffer.height / 2.0; // Middle of the screen vertically.
    float radius     = 50;                  // Nice, big circle.
    pax_simple_circle(&buffer, pax_col_rgb(255, 0, 0), midpoint_x, midpoint_y, radius);
    
    // White text.
    float text_x     = 0;                   // Top left corner of the screen.
    float text_y     = 0;                   // Top left corner of the screen.
    char *my_text    = "Hello, World!";     // You can pick any message you'd like.
    float text_size  = 18;                  // Twice the normal size for "7x9".
    pax_draw_text(&buffer, pax_col_rgb(255, 255, 255), PAX_FONT_DEFAULT, text_size, text_x, text_y, my_text);
    
    // Put it on the screen.
    if (ili9341_write(&display, buffer.buf)) {
        ESP_LOGE("my_tag", "Display write failed.");
    } else {
        ESP_LOGI("my_tag", "Display write success.");
    }
    
    // Cleanup.
    pax_buf_destroy(&buffer);
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
	// pax_buf_init(&image, image_empty, 25, 25, PAX_BUF_32_8888ARGB);
	pax_buf_init(&image, pallette_test_img, 5, 4, PAX_BUF_8_PAL);
	image.pallette = pallette_test_pal;
	image.pallette_size = sizeof(pallette_test_pal) / sizeof(pax_col_t);
	// ESP_LOGI(TAG, "Creating test buffer.");
	// pax_buf_t conv;
	// pax_buf_init(&conv, NULL, 25, 25, PAX_BUF_32_8888ARGB);
	// pax_buf_convert(&conv, &image, PAX_BUF_8_2222ARGB);
	
	// my_graphics_function();
	// return;
	
	pax_buf_t dummy;
	
	// Todo: Create file of memory.
	FILE *fd = NULL;
	pax_decode_png(&dummy, fd, PAX_BUF_1_GREY);
	
	// Send a test pattern.
	ESP_LOGI(TAG, "Creating framebuffer.");
	pax_buf_t buf;
	pax_buf_init(&buf, framebuffer, ILI9341_WIDTH, ILI9341_HEIGHT, PAX_BUF_16_565RGB);
	if (pax_last_error) {
		ESP_LOGE(TAG, "Framebuffer creation failed.");
		return;
	}
	
	// pax_debug(&conv);
	
	menu_entry_t menu_entries[] = {
		{ .text = "OHM2013" },
		{ .text = "SHA2017" },
		{ .text = "MCH2022" }
	};
	
	menu_t menu = {
		.n_entries = sizeof(menu_entries) / sizeof(menu_entry_t),
		.entries   = menu_entries,
		.font_size = 18
	};
	
	pax_shader_t kots = {
		.callback = regenboogkots
	};
	
	pax_background(&buf, 0xff000000);
	
	pax_push_2d(&buf);
	pax_pop_2d(&buf);
	
	while (1) {
		uint64_t millis = esp_timer_get_time() / 1000;
		
		pax_col_t color0 = pax_col_hsv(millis * 255 / 8000 + 127, 255, 255);
		pax_col_t color1 = pax_col_hsv(millis * 255 / 8000, 255, 255);
		
		// Single triangle demo.
		// uint32_t a = millis / 16;
		// pax_clip(&buf, 0, 0, buf.width-((a >> 2) % buf.width), buf.height-((a >> 1) % buf.height));
		// pax_simple_rect(&buf, 0xff000000, 0, 0, buf.width, buf.height);
		// pax_draw_tri(&buf, -1, 0, 0, a % buf.width, buf.height / 2.0, buf.width / 2.0, a % buf.height);
		
		// Shading demo.
		// pax_push_2d(&buf);
		// float a0 = millis / 3000.0 * M_PI;
		// pax_apply_2d(&buf, matrix_2d_rotate(a0));
		// pax_shade_rect(&buf, -1, &test_shader, NULL, -25, -25, 50, 50);
		// pax_shade_circle(&buf, -1, &test_shader, NULL, 0, 0, 25);
		// pax_pop_2d(&buf);
		
		// Epic arcs demo.
		float a0 = millis / 3000.0 * M_PI;
		float a1 = fmodf(a0, M_PI * 4) - M_PI * 2;
		float a2 = millis / 5000.0 * M_PI;
		pax_push_2d(&buf);
		pax_apply_2d(&buf, matrix_2d_translate(100 + (buf.width - 100) * 0.5, 20 + (buf.height - 20) * 0.5));
		pax_apply_2d(&buf, matrix_2d_scale(50, 50));
		
		pax_draw_rect(&buf, 0xff000000, -1.3, -1.3, 2.6, 2.6);
		
		pax_apply_2d(&buf, matrix_2d_rotate(-a2));
		// pax_shade_arc(&buf, color0, &kots, NULL, 0, 0, 1, a0 + a2, a0 + a1 + a2);
		// pax_shade_circle(&buf, -1, &PAX_SHADER_TEXTURE(&image), NULL, 0, 0, 1);
		pax_draw_arc(&buf, color0, 0, 0, 1, a0 + a2, a0 + a1 + a2);
		
		pax_apply_2d(&buf, matrix_2d_rotate(a0 + a2));
		pax_push_2d(&buf);
		pax_apply_2d(&buf, matrix_2d_translate(1, 0));
		pax_draw_rect(&buf, color1, -0.25, -0.25, 0.5, 0.5);
		pax_pop_2d(&buf);
		
		pax_apply_2d(&buf, matrix_2d_rotate(a1));
		pax_push_2d(&buf);
		pax_apply_2d(&buf, matrix_2d_translate(1, 0));
		pax_apply_2d(&buf, matrix_2d_rotate(-a0 - a1 + M_PI * 0.5));
		pax_draw_tri(&buf, color1, 0.25, 0, -0.125, 0.2165, -0.125, -0.2165);
		pax_pop_2d(&buf);
		
		pax_pop_2d(&buf);
		
		// Line test.
		// pax_simple_rect(&buf, 0xffff0000, 0, 0, 5, 5);
		// pax_simple_line(&buf, -1, 0, 0, 4, 4);
		// float max_x = 10.0 / buf.width;
		// float max_y = 10.0 / buf.height;
		// pax_quad_t rect_uv = {
		// 	.x0 = 0,     .y0 = 0,
		// 	.x1 = max_x, .y1 = 0,
		// 	.x2 = max_x, .y2 = max_y,
		// 	.x3 = 0,     .y3 = max_y
		// };
		// pax_shade_rect(&buf, -1, &PAX_SHADER_TEXTURE(&buf), &rect_uv, 10, 0, 50, 50);
		
		// Menu.
		char *text = "MCH2022 badge";
		pax_vec1_t size = pax_text_size(NULL, 18, text);
		pax_draw_text(&buf, color1, NULL, 18, (buf.width - size.x) / 2, 0, text);
		pax_simple_line(&buf, color1, 0, 19, buf.width - 1, 19);
		menu_render(&buf, &menu, 0, 20, 100, buf.height);
		
		if (ili9341_write(&display, framebuffer)) {
			ESP_LOGE(TAG, "Display write failed.");
			vTaskDelay(3000 / portTICK_PERIOD_MS);
			esp_restart();
		}
		
		// if (millis > 1500) break;
	}
}
