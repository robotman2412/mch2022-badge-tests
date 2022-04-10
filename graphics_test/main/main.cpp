
#include "include/main.h"

// The MENU.
#include "sample_menu.h"
#include "techdemo.h"

extern "C" {
extern void el_tech_demo();
extern void fpga_tests();
extern void name_tag();
extern void countdown();
extern void png_tests();
extern void gimmekeyboard();
extern void benchmark();
}

extern void html_test();

// #include "../components/pax-graphics/test-images/empty.c"
static const char *TAG = "main";

extern "C" {
PCA9555   dev_pca9555 = {0};
ICE40     dev_ice40   = {0};
ILI9341   display     = {0};

uint8_t   framebuffer[ILI9341_BUFFER_SIZE];
pax_buf_t buf;
pax_buf_t clip;
}

uint8_t   clipbuffer [(ILI9341_WIDTH * ILI9341_HEIGHT * PAX_GET_BPP(PAX_TD_BUF_TYPE) + 7) / 8];

// Wrapper functions for linking the ICE40 component to the PCA9555 component
esp_err_t ice40_get_done_wrapper (bool *done)  { return pca9555_get_gpio_value(&dev_pca9555, PCA9555_PIN_FPGA_CDONE, done);  }
esp_err_t ice40_set_reset_wrapper(bool  reset) { return pca9555_set_gpio_value(&dev_pca9555, PCA9555_PIN_FPGA_RESET, reset); }

typedef void (*mfunc_t)();

void setup_me_hardware() {
	esp_err_t res = 0;
	
    // Interrupts
    res = gpio_install_isr_service(0);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Initializing ISR service failed");
		vTaskDelay(3000 / portTICK_PERIOD_MS);
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
		vTaskDelay(3000 / portTICK_PERIOD_MS);
		esp_restart();
	} else {
		ESP_LOGI(TAG, "SPI bus initialised.");
	}
	
	res = i2c_init(I2C_BUS_SYS, GPIO_I2C_SYS_SDA, GPIO_I2C_SYS_SCL, I2C_SPEED_SYS, false, false);
	if (res != ESP_OK) {
		ESP_LOGE(TAG, "Initializing system I2C bus failed.");
		vTaskDelay(3000 / portTICK_PERIOD_MS);
		esp_restart();
	} else {
		ESP_LOGI(TAG, "I2C initialised.");
	}
	
	// Initialise PCA9555.
	res = pca9555_init(&dev_pca9555, I2C_BUS_SYS, PCA9555_ADDR, GPIO_INT_PCA9555);
	if (res != ESP_OK) {
		ESP_LOGE(TAG, "Initializing PCA9555 failed.");
		vTaskDelay(3000 / portTICK_PERIOD_MS);
		esp_restart();
	} else {
		ESP_LOGI(TAG, "PCA9555 initialised.");
	}
	
	res = pca9555_set_gpio_direction(&dev_pca9555, PCA9555_PIN_FPGA_RESET, true);
	if (res != ESP_OK) {
		ESP_LOGE(TAG, "Setting the FPGA reset pin on the PCA9555 to output failed");
		vTaskDelay(3000 / portTICK_PERIOD_MS);
		esp_restart();
	}
	
	res = pca9555_set_gpio_value(&dev_pca9555, PCA9555_PIN_FPGA_RESET, false);
	if (res != ESP_OK) {
		ESP_LOGE(TAG, "Setting the FPGA reset pin on the PCA9555 to low failed");
		vTaskDelay(3000 / portTICK_PERIOD_MS);
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
		vTaskDelay(3000 / portTICK_PERIOD_MS);
		esp_restart();
	} else {
		ESP_LOGI(TAG, "FPGA initialised.");
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
		vTaskDelay(3000 / portTICK_PERIOD_MS);
		esp_restart();
	} else {
		ESP_LOGI(TAG, "Display initialised.");
	}
	
}

extern "C" void app_main() {
	// Let's see.
	printf("I booted!\n");
	vTaskDelay(1000 / portTICK_PERIOD_MS);
	setup_me_hardware();
	
	// Initialise GFX.
	ESP_LOGI(TAG, "Creating framebuffer.");
	pax_buf_init(&buf, framebuffer, ILI9341_WIDTH, ILI9341_HEIGHT, PAX_BUF_16_565RGB);
	if (pax_last_error) {
		ESP_LOGE(TAG, "Framebuffer creation failed.");
		vTaskDelay(3000 / portTICK_PERIOD_MS);
		esp_restart();
	}
	ESP_LOGI(TAG, "Creating clip buffer.");
	pax_buf_init(&clip, clipbuffer, ILI9341_WIDTH, ILI9341_HEIGHT, PAX_TD_BUF_TYPE);
	if (pax_last_error) {
		ESP_LOGE(TAG, "Clip buffer creation failed.");
		vTaskDelay(3000 / portTICK_PERIOD_MS);
		esp_restart();
	}
	
	menu_entry_t menu_entries[] = {
		{ .text = "Tech demo" },
		{ .text = "Benchmark" },
		{ .text = "FPGA test" },
		{ .text = "Name tag" },
		{ .text = "HTML test" },
		{ .text = "Countdown" },
		{ .text = "PNG test" },
		{ .text = "Keyboard" },
	};
	mfunc_t menu_functions[] = {
		el_tech_demo,
		benchmark,
		fpga_tests,
		name_tag,
		html_test,
		countdown,
		png_tests,
		gimmekeyboard,
	};
	size_t menu_size = sizeof(menu_entries) / sizeof(menu_entry_t);
	menu_t menu = {
		.n_entries = menu_size,
		.entries   = menu_entries,
		.font_size = 18
	};
	
	pax_background(&buf, 0xff000000);
	int selection  = 0;
	bool last_up   = false;
	bool last_down = false;
	
	// Up the frequency.
	esp_pm_lock_handle_t pm_lock;
	esp_pm_lock_create(ESP_PM_CPU_FREQ_MAX, 0, NULL, &pm_lock);
	esp_pm_lock_acquire(pm_lock);
	pax_enable_multicore(1);
	
	while (1) {
		uint64_t millis  = esp_timer_get_time() / 1000;
		
		pax_col_t color0 = pax_col_hsv(millis * 255 / 8000 + 127, 255, 255);
		pax_col_t color1 = pax_col_hsv(millis * 255 / 8000, 255, 255);
		pax_background(&buf, 0xff000000);
		
		// Epic arcs demo.
		float a0 = millis / 5000.0 * M_PI;
		float a1 = fmodf(a0, M_PI * 4) - M_PI * 2;
		float a2 = millis / 8000.0 * M_PI;
		pax_push_2d(&buf);
		pax_apply_2d(&buf, matrix_2d_translate(150 + (buf.width - 150) * 0.5f, 20 + (buf.height - 20) * 0.5f));
		pax_apply_2d(&buf, matrix_2d_scale(50, 50));
		
		pax_apply_2d(&buf, matrix_2d_rotate(-a2));
		pax_draw_arc(&buf, color0, 0, 0, 1, a0 + a2, a0 + a1 + a2);
		
		pax_apply_2d(&buf, matrix_2d_rotate(a0 + a2));
		pax_push_2d(&buf);
		pax_apply_2d(&buf, matrix_2d_translate(1, 0));
		pax_draw_rect(&buf, color1, -0.25f, -0.25f, 0.5f, 0.5f);
		pax_pop_2d(&buf);
		
		pax_apply_2d(&buf, matrix_2d_rotate(a1));
		pax_push_2d(&buf);
		pax_apply_2d(&buf, matrix_2d_translate(1, 0));
		pax_apply_2d(&buf, matrix_2d_rotate(-a0 - a1 + M_PI * 0.5));
		pax_draw_tri(&buf, color1, 0.25f, 0.0f, -0.125f, 0.2165f, -0.125f, -0.2165f);
		pax_pop_2d(&buf);
		
		pax_pop_2d(&buf);
		
		// Of non intersnect.
		// pax_vec1_t points[] = {
		// 	(pax_vec1_t) { .x = -1.0f, .y = -1.0f },
		// 	(pax_vec1_t) { .x =  0.0f, .y = -0.5f },
		// 	(pax_vec1_t) { .x =  1.0f, .y = -1.0f },
		// 	(pax_vec1_t) { .x =  1.0f, .y =  1.0f },
		// 	(pax_vec1_t) { .x =  0.0f, .y =  0.5f },
		// 	(pax_vec1_t) { .x = -1.0f, .y =  1.0f },
		// };
		// Of yesm intersnect.
		// pax_vec1_t points[] = {
		// 	(pax_vec1_t) { .x = -1.0f, .y = -1.0f },
		// 	(pax_vec1_t) { .x =  0.0f, .y =  0.5f },
		// 	(pax_vec1_t) { .x =  1.0f, .y = -1.0f },
		// 	(pax_vec1_t) { .x =  1.0f, .y =  1.0f },
		// 	(pax_vec1_t) { .x =  0.0f, .y = -0.5f },
		// 	(pax_vec1_t) { .x = -1.0f, .y =  1.0f },
		// };
		// Of torture test.
		// pax_vec1_t points[] = {
		// 	(pax_vec1_t) { .x = -1.0f, .y =  1.0f },
		// 	(pax_vec1_t) { .x =  0.0f, .y =  0.3f },
		// 	(pax_vec1_t) { .x =  1.0f, .y =  1.0f },
		// 	(pax_vec1_t) { .x =  1.0f, .y = -0.3f },
		// 	(pax_vec1_t) { .x =  0.0f, .y = -1.0f },
		// 	(pax_vec1_t) { .x = -1.0f, .y = -0.3f },
		// };
		// size_t n_points = sizeof(points) / sizeof(pax_vec1_t);
		// pax_push_2d(&buf);
		// pax_apply_2d(&buf, matrix_2d_translate(150 + (buf.width - 150) * 0.5f, 20 + (buf.height - 20) * 0.5f));
		// pax_apply_2d(&buf, matrix_2d_scale(50, 50));
		// // El test triagnulbaste of.
		// pax_draw_shape(&buf, 0xffff0000, n_points, points);
		// pax_outline_shape(&buf, 0xff00ff00, n_points, points);
		// pax_pop_2d(&buf);
		
		
		// GET THE BUTTONS!
		bool up   = last_up;
		bool down = last_down;
		bool acc  = false;
		pca9555_get_gpio_value(&dev_pca9555, PCA9555_PIN_BTN_JOY_UP,   &up);
		pca9555_get_gpio_value(&dev_pca9555, PCA9555_PIN_BTN_JOY_DOWN, &down);
		pca9555_get_gpio_value(&dev_pca9555, PCA9555_PIN_BTN_ACCEPT,   &acc);
		if (acc) {
			// PERFORM!
			menu_functions[selection]();
			continue;
		} else if (up && !last_up) {
			// Going UP!
			if (selection <= 0) {
				selection = menu_size - 1;
			} else {
				selection --;
			}
		} else if (down && !last_down) {
			// Going down...
			if (selection >= menu_size - 1) {
				selection = 0;
			} else {
				selection ++;
			}
		}
		last_up   = up;
		last_down = down;
		
		// Menu.
		char *text = "PAX test firmware";
		pax_vec1_t size = pax_text_size(NULL, 18, text);
		pax_draw_text(&buf, color1, NULL, 18, (buf.width - size.x) / 2, 0, text);
		pax_simple_line(&buf, color1, 0, 19, buf.width - 1, 19);
		menu_render(&buf, &menu, selection, color1, 0, 20, 150, buf.height);
		
		// Barf it out.
		ili9341_write(&display, framebuffer);
		taskYIELD();
	}
	
	// Cleaning.
	esp_pm_lock_release(pm_lock);
	esp_pm_lock_delete(pm_lock);
}
