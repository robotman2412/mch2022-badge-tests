
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
#include "fake_file.h"

// Menu.
#include "sample_menu.h"

#include "techdemo.h"

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

#include <esp_vfs_fat.h>
unsigned char png_test_png[] = {
	0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d,
	0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x0a,
	0x08, 0x02, 0x00, 0x00, 0x00, 0x02, 0x50, 0x58, 0xea, 0x00, 0x00, 0x00,
	0x09, 0x70, 0x48, 0x59, 0x73, 0x00, 0x00, 0x2e, 0x23, 0x00, 0x00, 0x2e,
	0x23, 0x01, 0x78, 0xa5, 0x3f, 0x76, 0x00, 0x00, 0x01, 0x41, 0x49, 0x44,
	0x41, 0x54, 0x18, 0x19, 0x01, 0x36, 0x01, 0xc9, 0xfe, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff,
	0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
	0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2c, 0x4e, 0x17, 0xf3, 0x25,
	0x0d, 0x36, 0x6c, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae,
	0x42, 0x60, 0x82
};
size_t png_test_png_len = 399;

uint8_t framebuffer[ILI9341_BUFFER_SIZE];

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
	ESP_LOGI(TAG, "Loading test image.");
	pax_buf_t image;
	// pax_buf_init(&image, image_empty, 25, 25, PAX_BUF_32_8888ARGB);
	// pax_buf_init(&image, pallette_test_img, 5, 4, PAX_BUF_8_PAL);
	// image.pallette = pallette_test_pal;
	// image.pallette_size = sizeof(pallette_test_pal) / sizeof(pax_col_t);
	
	// Mount FATFS.
	// ESP_LOGI(TAG, "Mounting filesystem.");
	// FATFS *lolwut;
	// // esp_err_t fatty = esp_vfs_fat_register("/__spiflash", "", 10, &lolwut);
	// esp_vfs_fat_mount_config_t fatty_config = {
	// 	.format_if_mount_failed = true,
	// 	.allocation_unit_size = CONFIG_WL_SECTOR_SIZE,
	// 	.max_files = 20
	// };
	// wl_handle_t myhandle = WL_INVALID_HANDLE;
	// esp_err_t fatty = esp_vfs_fat_spiflash_mount("/__spiflash", "storage", &fatty_config, &myhandle);
	// if (fatty) {
	// 	ESP_LOGE(TAG, "FATFS error 0x%x", fatty);
	// 	return;
	// }
	
	// Todo: Create file of memory.
	// FILE *fd = fopen("/__spiflash/test_img.png", "w+");
	// ESP_LOGW(TAG, "lmao is %p", fd);
	// fwrite(png_test_png, 1, png_test_png_len, fd);
	// fseek(fd, 0, SEEK_SET);
	// pax_decode_png(&dummy, fd, PAX_BUF_1_GREY);
	
	// Send a test pattern.
	ESP_LOGI(TAG, "Creating framebuffer.");
	pax_buf_t buf;
	pax_buf_init(&buf, framebuffer, ILI9341_WIDTH, ILI9341_HEIGHT, PAX_BUF_16_565RGB);
	if (pax_last_error) {
		ESP_LOGE(TAG, "Framebuffer creation failed.");
		return;
	}
	// pax_buf_t clip;
	// ESP_LOGI(TAG, "Creating clip buffer.");
	// pax_buf_init(&clip, NULL, ILI9341_WIDTH, ILI9341_HEIGHT, PAX_TD_BUF_TYPE);
	// if (pax_last_error) {
	// 	ESP_LOGE(TAG, "Clip buffer creation failed.");
	// 	return;
	// }
	
	// pax_techdemo_init(&buf, &clip);
	// uint64_t start = esp_timer_get_time() / 1000;
	// char text_buf[32];
	// while (1) {
	// 	uint64_t pre = esp_timer_get_time();
	// 	uint64_t now = esp_timer_get_time() / 1000 - start;
	// 	bool fin = pax_techdemo_draw(now);
	// 	uint64_t post = esp_timer_get_time();
	// 	int fps = 1000000 / (post - pre);
	// 	snprintf(text_buf, 31, "%d FPS", fps);
	// 	pax_vec1_t text_size = pax_text_size(PAX_FONT_DEFAULT, 9, text_buf);
	// 	pax_draw_text(&buf, -1, PAX_FONT_DEFAULT, 9, buf.width - text_size.x - 1, 0, text_buf);
		
	// 	if (ili9341_write(&display, framebuffer)) {
	// 		ESP_LOGE(TAG, "Display write failed.");
	// 		vTaskDelay(3000 / portTICK_PERIOD_MS);
	// 		esp_restart();
	// 	}
	// 	if (fin) {
	// 		vTaskDelay(5000 / portTICK_PERIOD_MS);
	// 		pax_techdemo_init(&buf, &clip);
	// 		start = esp_timer_get_time() / 1000;
	// 	}
	// }
	// return;
	
	pax_buf_t png_test_buf;
	FILE *png_fd = xopenmem(png_test_png, png_test_png_len);
	pax_decode_png(&png_test_buf, png_fd, PAX_BUF_32_8888ARGB);
	pax_background(&png_test_buf, 0);
	xclose(png_fd);
	
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
	
	// pax_background(&buf, 0xff000000);
	pax_background(&buf, 0xffffffff);
	
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
		
		// Gimme PNG.
		pax_push_2d(&buf);
		// pax_apply_2d(&buf, matrix_2d_translate(100 + (buf.width - 100) * 0.5, 20 + (buf.height - 20) * 0.5));
		pax_apply_2d(&buf, matrix_2d_scale(10, 10));
		pax_shade_rect(&buf, -1, &PAX_SHADER_TEXTURE(&png_test_buf), NULL, 0, 0, png_test_buf.width, png_test_buf.height);
		pax_pop_2d(&buf);
		
		// Epic arcs demo.
		// float a0 = millis / 3000.0 * M_PI;
		// float a1 = fmodf(a0, M_PI * 4) - M_PI * 2;
		// float a2 = millis / 5000.0 * M_PI;
		// pax_push_2d(&buf);
		// pax_apply_2d(&buf, matrix_2d_translate(100 + (buf.width - 100) * 0.5, 20 + (buf.height - 20) * 0.5));
		// pax_apply_2d(&buf, matrix_2d_scale(50, 50));
		
		// pax_draw_rect(&buf, 0xff000000, -1.3, -1.3, 2.6, 2.6);
		
		// pax_apply_2d(&buf, matrix_2d_rotate(-a2));
		// // pax_shade_arc(&buf, color0, &kots, NULL, 0, 0, 1, a0 + a2, a0 + a1 + a2);
		// // pax_shade_circle(&buf, -1, &PAX_SHADER_TEXTURE(&image), NULL, 0, 0, 1);
		// pax_draw_arc(&buf, color0, 0, 0, 1, a0 + a2, a0 + a1 + a2);
		
		// pax_apply_2d(&buf, matrix_2d_rotate(a0 + a2));
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
		// char *text = "MCH2022 badge";
		// pax_vec1_t size = pax_text_size(NULL, 18, text);
		// pax_draw_text(&buf, color1, NULL, 18, (buf.width - size.x) / 2, 0, text);
		// pax_simple_line(&buf, color1, 0, 19, buf.width - 1, 19);
		// menu_render(&buf, &menu, 0, 20, 100, buf.height);
		
		if (ili9341_write(&display, framebuffer)) {
			ESP_LOGE(TAG, "Display write failed.");
			vTaskDelay(3000 / portTICK_PERIOD_MS);
			esp_restart();
		}
		
		// if (millis > 1500) break;
	}
}
