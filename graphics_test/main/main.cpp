
#include "include/main.h"

// The MENU.
#include "sample_menu.h"
#include "techdemo.h"

extern "C" {
static uint8_t rp2040_fw_version = 0;
xSemaphoreHandle i2c_semaphore = NULL;
uint16_t button_bits = 0;

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
RP2040    dev_rp2040  = {0};

uint8_t   framebuffer[ILI9341_BUFFER_SIZE];
pax_buf_t buf;
pax_buf_t clip;
}

uint8_t   clipbuffer [(ILI9341_WIDTH * ILI9341_HEIGHT * PAX_GET_BPP(PAX_TD_BUF_TYPE) + 7) / 8];

// Wrapper functions for linking CRACK together.
esp_err_t ice40_get_done_wrapper(bool* done) {
	uint16_t  buttons;
	esp_err_t res = rp2040_read_buttons(&dev_rp2040, &buttons);
	if (res != ESP_OK) return res;
	*done = !((buttons >> 5) & 0x01);
	return ESP_OK;
}

esp_err_t ice40_set_reset_wrapper(bool reset) {
	esp_err_t res = rp2040_set_fpga(&dev_rp2040, reset);
	vTaskDelay(100 / portTICK_PERIOD_MS);
	return res;
}

void ili9341_set_lcd_mode(bool mode) {
    ESP_LOGI(TAG, "LCD mode switch to %s", mode ? "FPGA" : "ESP32");
    esp_err_t res = gpio_set_level((gpio_num_t) GPIO_LCD_MODE, mode);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Setting LCD mode failed");
    }
}

typedef void (*mfunc_t)();

void setup_me_hardware() {
	esp_err_t res = 0;
	// vTaskDelay(3000 / portTICK_PERIOD_MS);
	
    // Interrupts
    res = gpio_install_isr_service(0);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "Initializing ISR service failed.");
		vTaskDelay(3000 / portTICK_PERIOD_MS);
		esp_restart();
    }
	
	// I2C bus
	i2c_config_t i2c_config = {
		.mode = I2C_MODE_MASTER,
		.sda_io_num = GPIO_I2C_SDA,
		.scl_io_num = GPIO_I2C_SCL,
		.sda_pullup_en = false,
		.scl_pullup_en = false,
		.master = { .clk_speed = I2C_SPEED, },
		.clk_flags = 0
	};
	
	res = i2c_param_config(I2C_BUS, &i2c_config);
	if (res != ESP_OK) {
		ESP_LOGE(TAG, "Configuring I2C bus parameters failed");
		vTaskDelay(3000 / portTICK_PERIOD_MS);
		esp_restart();
	}
	
	res = i2c_set_timeout(I2C_BUS, I2C_TIMEOUT * 80);
	if (res != ESP_OK) {
		ESP_LOGE(TAG, "Configuring I2C bus timeout failed");
		vTaskDelay(3000 / portTICK_PERIOD_MS);
		esp_restart();
	}
	
	res = i2c_driver_install(I2C_BUS, i2c_config.mode, 0, 0, 0);
	if (res != ESP_OK) {
		ESP_LOGE(TAG, "Initializing system I2C bus failed");
		vTaskDelay(3000 / portTICK_PERIOD_MS);
		esp_restart();
	}
	
	i2c_semaphore = xSemaphoreCreateBinary();
	xSemaphoreGive( i2c_semaphore );
	
	// Initialise SPI bus.
	spi_bus_config_t spiConfig = {
		.mosi_io_num = GPIO_SPI_MOSI,
		.miso_io_num = GPIO_SPI_MISO,
		.sclk_io_num = GPIO_SPI_CLK,
		.quadwp_io_num = -1,
		.quadhd_io_num = -1,
		.max_transfer_sz = SPI_MAX_TRANSFER_SIZE
	};
	if (spi_bus_initialize(SPI_BUS, &spiConfig, SPI_DMA_CHANNEL)) {
		ESP_LOGE(TAG, "SPI bus initialisation failed.");
		vTaskDelay(3000 / portTICK_PERIOD_MS);
		esp_restart();
	} else {
		ESP_LOGI(TAG, "SPI bus initialised.");
	}
	
	// Initialise RP2040.
	dev_rp2040.i2c_bus       = I2C_BUS;
	dev_rp2040.i2c_address   = RP2040_ADDR;
	dev_rp2040.pin_interrupt = GPIO_INT_RP2040;
	dev_rp2040.queue         = xQueueCreate(8, sizeof(rp2040_input_message_t));
	dev_rp2040.i2c_semaphore = i2c_semaphore;

	res = rp2040_init(&dev_rp2040);
	if (res != ESP_OK) {
		ESP_LOGE(TAG, "Initializing RP2040 failed.");
		vTaskDelay(3000 / portTICK_PERIOD_MS);
		esp_restart();
	} else {
		ESP_LOGI(TAG, "RP2040 initialised.");
	}

	if (rp2040_get_firmware_version(&dev_rp2040, &rp2040_fw_version) != ESP_OK) {
		ESP_LOGE(TAG, "Failed to read RP2040 firmware version.");
		vTaskDelay(3000 / portTICK_PERIOD_MS);
		esp_restart();
	}
	
	// Initialise FPGA.
	dev_ice40.spi_bus               = SPI_BUS;
	dev_ice40.pin_cs                = GPIO_SPI_CS_FPGA;
	dev_ice40.pin_done              = -1;
	dev_ice40.pin_reset             = -1;
	dev_ice40.pin_int               = GPIO_INT_FPGA;
	dev_ice40.spi_speed_full_duplex = 26700000;
	dev_ice40.spi_speed_half_duplex = 40000000;
	dev_ice40.spi_speed_turbo       = 80000000;
	dev_ice40.spi_input_delay_ns    = 10;
	dev_ice40.spi_max_transfer_size = SPI_MAX_TRANSFER_SIZE;
	dev_ice40.get_done              = ice40_get_done_wrapper;
	dev_ice40.set_reset             = ice40_set_reset_wrapper;
	
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
	display.pin_cs     = GPIO_SPI_CS_LCD;
	display.pin_dcx    = GPIO_SPI_DC_LCD;
	display.pin_reset  = GPIO_LCD_RESET;
	display.rotation   = 1;
	display.color_mode = true;
	display.spi_speed  = 60000000; // 60 MHz
	display.spi_max_transfer_size = SPI_MAX_TRANSFER_SIZE;
	display.callback   = ili9341_set_lcd_mode; // No callbacks just yet.
	
	res = gpio_set_direction((gpio_num_t) GPIO_LCD_MODE, GPIO_MODE_OUTPUT);
	if (res != ESP_OK) {
		ESP_LOGE(TAG, "Initializing LCD mode GPIO failed");
		vTaskDelay(3000 / portTICK_PERIOD_MS);
		esp_restart();
	}
	
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
		
		/*
pca9555_get_gpio_value\(.*?,\s*PCA9555_PIN_(BTN.*?),\s*&(.*?)\).*?;
$2 = !!(button_bits & MASK_$1);
		*/
		// GET THE BUTTONS!
		bool up   = last_up;
		bool down = last_down;
		bool acc  = false;
		rp2040_read_buttons(&dev_rp2040, &button_bits);
		up   = !!(button_bits & MASK_BTN_JOY_UP);
		down = !!(button_bits & MASK_BTN_JOY_DOWN);
		acc  = !!(button_bits & MASK_BTN_ACCEPT);
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
		char *text = "MCH2022 ¯\\_(ツ)_/¯ firmware"; //"PAX test firmware";
		pax_font_t *font      = pax_get_font("sky");
		float       font_size = 18;
		pax_vec1_t size = pax_text_size(font, font_size, text);
		pax_draw_text(&buf, color1, font, font_size, (buf.width - size.x) / 2, 0, text);
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
