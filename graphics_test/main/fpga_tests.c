
#include "include/main.h"

// #include "../fpga/fpga_test.h"
#include "include/fpga_lcd_bin.h"

static const char *TAG = "fpga-tests";

void set_parallel_mode(bool yes) {
	uint8_t value_of = yes;
	i2c_write_reg_n(RP2040_I2C_BUS, RP2040_I2C_ADDR, RP2040_REG_LCDM, &value_of, 1);
	size_t limit = esp_timer_get_time() + 100000;
	value_of = !yes;
	while (value_of != yes && esp_timer_get_time() < limit) {
		i2c_read_reg(RP2040_I2C_BUS, RP2040_I2C_ADDR, RP2040_REG_LCDM, &value_of, 1);
	}
}

static void fpga_do_lcd() {
	
	ili9341_deinit(&display);
	set_parallel_mode(true);
	
	ice40_enable(&dev_ice40);
	esp_err_t res = ice40_load_bitstream(&dev_ice40, fpga_test_bin, fpga_test_bin_len);
	if (res != ESP_OK) {
		ESP_LOGE(TAG, "Writing bitstream failed.");
		vTaskDelay(3000 / portTICK_PERIOD_MS);
		esp_restart();
	} else {
		ESP_LOGI(TAG, "Bitstream written.");
	}
	
	while (1) {
		// Limit of loop.
		bool exuent = false;
		pca9555_get_gpio_value(&dev_pca9555, PCA9555_PIN_BTN_HOME, &exuent);
		if (exuent) break;
	}
	ice40_disable(&dev_ice40);
	
	set_parallel_mode(false);
	if (ili9341_init(&display)) {
		ESP_LOGE(TAG, "Display reinitialisation failed.");
		vTaskDelay(3000 / portTICK_PERIOD_MS);
		esp_restart();
	} else {
		ESP_LOGI(TAG, "Display reinitialised.");
	}
}

static void fpga_do_spi() {
	
	esp_err_t res = ice40_load_bitstream(&dev_ice40, fpga_test_bin, fpga_test_bin_len);
	if (res != ESP_OK) {
		ESP_LOGE(TAG, "Writing bitstream failed.");
		vTaskDelay(3000 / portTICK_PERIOD_MS);
		esp_restart();
	} else {
		ESP_LOGI(TAG, "Bitstream written.");
	}
	
	// Lettuce SPI:FPGA.
	bool a_button = false;
	bool b_button = false;
	pca9555_get_gpio_value(&dev_pca9555, PCA9555_PIN_BTN_ACCEPT, &a_button);
	pca9555_get_gpio_value(&dev_pca9555, PCA9555_PIN_BTN_BACK, &b_button);
	char not_a_buffer[256];
	// Clear.
	pax_background(&buf, 0);
	if (ili9341_write(&display, framebuffer)) {
		ESP_LOGE(TAG, "Display write failed.");
		vTaskDelay(3000 / portTICK_PERIOD_MS);
		esp_restart();
	}
	while (1) {
		// What the A BUTTON?
		bool new_a_button = a_button;
		bool new_b_button = b_button;
		pca9555_get_gpio_value(&dev_pca9555, PCA9555_PIN_BTN_ACCEPT, &new_a_button);
		pca9555_get_gpio_value(&dev_pca9555, PCA9555_PIN_BTN_BACK, &new_b_button);
		
		if (new_a_button != a_button || new_b_button != b_button) {
			// Create a transaction of this le funneighx.
			uint8_t tx = 0;
			uint8_t rx;
			if (new_a_button && !a_button) {
				tx = 0x08;
			} else if (new_b_button && !b_button) {
				tx = 0x24;
			}
			a_button = new_a_button;
			b_button = new_b_button;
			if (tx) {
				// Perform exchange.
				ice40_transaction(&dev_ice40, &tx, 1, &rx, 1);
				
				// Show result.
				*not_a_buffer = 0;
				snprintf(not_a_buffer, 255, "Transaction:\ntx 0x%02x\nrx 0x%02x", tx, rx);
				pax_background(&buf, 0);
				pax_draw_text(&buf, -1, NULL, 9*3, 0, 0, not_a_buffer);
				
				// Flush.
				if (ili9341_write(&display, framebuffer)) {
					ESP_LOGE(TAG, "Display write failed.");
					vTaskDelay(3000 / portTICK_PERIOD_MS);
					esp_restart();
				}
			}
		}
		
		// Limit of loop.
		bool exuent = false;
		pca9555_get_gpio_value(&dev_pca9555, PCA9555_PIN_BTN_HOME, &exuent);
		if (exuent) break;
	}
}

void fpga_tests() {
	
	fpga_do_lcd();
	
}
