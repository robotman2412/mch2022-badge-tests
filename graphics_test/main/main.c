
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

#include <managed_i2c.h>

// I/O expander.
#include <pca9555.h>

// FPGA.
#include <ice40.h>

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

// DIY configuration.
// #define SPI_MOSI 12
// #define SPI_MISO 13
// #define SPI_SCLK 21
// #define SPI_MAX_TRANSFER 4094
// #define SPI_BUS ((spi_host_device_t) 2) //VSPI_HOST
// #define SPI_DMA_CH 2

// #define DISPLAY_RST 4
// #define DISPLAY_CS 16
// #define DISPLAY_DCX 17

// #include "../components/pax-graphics/test-images/empty.c"
#include "../fpga/fpga_test.h"

PCA9555 dev_pca9555 = {0};
ICE40   dev_ice40 = {0};
ILI9341 display = {0};

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
// uint8_t *framebuffer = NULL;

static const char *TAG = "main";

// Wrapper functions for linking the ICE40 component to the PCA9555 component
esp_err_t ice40_get_done_wrapper(bool* done) { return pca9555_get_gpio_value(&dev_pca9555, PCA9555_PIN_FPGA_CDONE, done); }
esp_err_t ice40_set_reset_wrapper(bool reset) { return pca9555_set_gpio_value(&dev_pca9555, PCA9555_PIN_FPGA_RESET, reset); }

void app_main() {
	
	// Let's see.
	printf("I booted!\n");
	
	vTaskDelay(5000 / portTICK_PERIOD_MS);
	
	esp_err_t res = 0;
	
    // Interrupts
    res = gpio_install_isr_service(0);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Initializing ISR service failed");
		esp_restart();
    }
	
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
	
	res = i2c_init(I2C_BUS_SYS, GPIO_I2C_SYS_SDA, GPIO_I2C_SYS_SCL, I2C_SPEED_SYS, false, false);
	if (res != ESP_OK) {
		ESP_LOGE(TAG, "Initializing system I2C bus failed.");
		esp_restart();
	} else {
		ESP_LOGI(TAG, "I2C initialised.");
	}
	
	// Initialise PCA9555.
	res = pca9555_init(&dev_pca9555, I2C_BUS_SYS, PCA9555_ADDR, GPIO_INT_PCA9555);
	if (res != ESP_OK) {
		ESP_LOGE(TAG, "Initializing PCA9555 failed.");
		esp_restart();
	} else {
		ESP_LOGI(TAG, "PCA9555 initialised.");
	}
	
	res = pca9555_set_gpio_direction(&dev_pca9555, PCA9555_PIN_FPGA_RESET, true);
	if (res != ESP_OK) {
		ESP_LOGE(TAG, "Setting the FPGA reset pin on the PCA9555 to output failed");
		esp_restart();
	}
	
	res = pca9555_set_gpio_value(&dev_pca9555, PCA9555_PIN_FPGA_RESET, false);
	if (res != ESP_OK) {
		ESP_LOGE(TAG, "Setting the FPGA reset pin on the PCA9555 to low failed");
		esp_restart();
	}
	
	pca9555_set_gpio_polarity(&dev_pca9555, PCA9555_PIN_BTN_START, true);
	pca9555_set_gpio_polarity(&dev_pca9555, PCA9555_PIN_BTN_SELECT, true);
	pca9555_set_gpio_polarity(&dev_pca9555, PCA9555_PIN_BTN_MENU, true);
	pca9555_set_gpio_polarity(&dev_pca9555, PCA9555_PIN_BTN_HOME, true);
	pca9555_set_gpio_polarity(&dev_pca9555, PCA9555_PIN_BTN_JOY_LEFT, true);
	pca9555_set_gpio_polarity(&dev_pca9555, PCA9555_PIN_BTN_JOY_PRESS, true);
	pca9555_set_gpio_polarity(&dev_pca9555, PCA9555_PIN_BTN_JOY_DOWN, true);
	pca9555_set_gpio_polarity(&dev_pca9555, PCA9555_PIN_BTN_JOY_UP, true);
	pca9555_set_gpio_polarity(&dev_pca9555, PCA9555_PIN_BTN_JOY_RIGHT, true);
	pca9555_set_gpio_polarity(&dev_pca9555, PCA9555_PIN_BTN_BACK, true);
	pca9555_set_gpio_polarity(&dev_pca9555, PCA9555_PIN_BTN_ACCEPT, true);
	
	// Reset all pin states so that the interrupt function doesn't trigger all the handlers because we inverted the polarity.
	dev_pca9555.pin_state = 0;
	
	// Initialise FPGA.
	dev_ice40.spi_bus = SPI_BUS;
	dev_ice40.pin_cs = SPI_CS_FPGA;
	dev_ice40.pin_done = -1;
	dev_ice40.pin_reset = -1;
	dev_ice40.pin_int = GPIO_INT_FPGA;
	dev_ice40.spi_speed = 23000000; // 23MHz
	dev_ice40.spi_max_transfer_size = SPI_MAX_TRANSFER;
	dev_ice40.get_done = ice40_get_done_wrapper;
	dev_ice40.set_reset = ice40_set_reset_wrapper;
	
	res = ice40_init(&dev_ice40);
	if (res != ESP_OK) {
		ESP_LOGE(TAG, "Initializing FPGA failed.");
		esp_restart();
	} else {
		ESP_LOGI(TAG, "FPGA initialised.");
	}
	
	// res = ice40_load_bitstream(&dev_ice40, fpga_test_bin, fpga_test_bin_len);
	// if (res != ESP_OK) {
	// 	ESP_LOGE(TAG, "Writing bitstream failed.");
	// 	esp_restart();
	// } else {
	// 	ESP_LOGI(TAG, "Bitstream written.");
	// }
	
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
	
	// Send a test pattern.
	ESP_LOGI(TAG, "Creating framebuffer.");
	pax_buf_t buf;
	pax_buf_init(&buf, framebuffer, ILI9341_WIDTH, ILI9341_HEIGHT, PAX_BUF_16_565RGB);
	if (pax_last_error) {
		ESP_LOGE(TAG, "Framebuffer creation failed.");
		return;
	}
	pax_buf_t clip;
	ESP_LOGI(TAG, "Creating clip buffer.");
	pax_buf_init(&clip, NULL, ILI9341_WIDTH, ILI9341_HEIGHT, PAX_TD_BUF_TYPE);
	if (pax_last_error) {
		ESP_LOGE(TAG, "Clip buffer creation failed.");
		return;
	}
	// Up the frequency.
	esp_pm_lock_handle_t pm_lock;
	esp_pm_lock_create(ESP_PM_CPU_FREQ_MAX, 0, NULL, &pm_lock);
	esp_pm_lock_acquire(pm_lock);
	
	pax_techdemo_init(&buf, &clip);
	uint64_t start = esp_timer_get_time() / 1000;
	uint64_t last_time = start;
	char text_buf[32];
	pax_enable_multicore(1);
	while (1) {
		uint64_t now = esp_timer_get_time() / 1000 - start;
		bool fin = pax_techdemo_draw(now);
		int fps = 1000 / (now - last_time);
		snprintf(text_buf, 31, "%d FPS", fps);
		pax_vec1_t text_size = pax_text_size(PAX_FONT_DEFAULT, 9, text_buf);
		pax_draw_text(&buf, -1, PAX_FONT_DEFAULT, 9, buf.width - text_size.x - 1, 0, text_buf);
		
		if (ili9341_write(&display, framebuffer)) {
			ESP_LOGE(TAG, "Display write failed.");
			vTaskDelay(3000 / portTICK_PERIOD_MS);
			esp_restart();
		}
		if (fin) {
			vTaskDelay(1000 / portTICK_PERIOD_MS);
			pax_techdemo_init(&buf, &clip);
			start = esp_timer_get_time() / 1000;
		}
		last_time = now;
	}
	
	// Cleaning.
	esp_pm_lock_release(pm_lock);
	esp_pm_lock_delete(pm_lock);
	return;
	
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
	
	ESP_LOGI(TAG, "PNG is %dx%d", png_test_buf.width, png_test_buf.height);
	
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
