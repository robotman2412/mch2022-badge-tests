
#include "include/main.h"

#include "../fpga/fpga_test.h"
// #include "include/fpga_lcd_bin.h"

static const uint8_t lcd_init_rom[] = {
	/* ==== Init ROM ==== */
	// 0, 0xef,
	// 1, 0x03,
	// 1, 0x80,
	// 1, 0x02,
	// 0, 0xcf, // Power control B
	// 1, 0x00,
	// 1, 0xc1,
	// 1, 0x30,
	// 0, 0xed, // Power on sequence control
	// 1, 0x64,
	// 1, 0x03,
	// 1, 0x12,
	// 1, 0x81,
	// 0, 0xe8, // Driver timing control A
	// 1, 0x85,
	// 1, 0x00,
	// 1, 0x78,
	// 0, 0xcb, // Power control A
	// 1, 0x39,
	// 1, 0x2c,
	// 1, 0x00,
	// 1, 0x34,
	// 1, 0x02,
	// 0, 0xf7, // Pump ratio control
	// 1, 0x20,
	// 0, 0xea, // Driver timing control B
	// 1, 0x00,
	// 1, 0x00,
	// 0, 0xc0, // Power control 1
	// 1, 0x23,
	// 0, 0xc1, // Power control 2
	// 1, 0x10,
	// 0, 0xc5, // VCOM Control 1
	// 1, 0x3e,
	// 1, 0x28,
	// 0, 0xc7, // VCOM Control 2
	// 1, 0x86,
	// 0, 0x36, // Memory Access Control
	// 1, 0x48,
	// 0, 0x3a,
	// 1, 0x55,
	// 0, 0xb1,
	// 1, 0x00,
	// 1, 0x18,
	// 0, 0xb6,
	// 1, 0x08,
	// 1, 0x82,
	// 1, 0x27,
	// 0, 0xf2,
	// 1, 0x00,
	// 0, 0x26,
	// 1, 0x01,
	// 0, 0xe0,
	// 1, 0x0f,
	// 1, 0x31,
	// 1, 0x2b,
	// 1, 0x0c,
	// 1, 0x0e,
	// 1, 0x08,
	// 1, 0x4e,
	// 1, 0xf1,
	// 1, 0x37,
	// 1, 0x07,
	// 1, 0x10,
	// 1, 0x03,
	// 1, 0x0e,
	// 1, 0x09,
	// 1, 0x00,
	// 0, 0xe1,
	// 1, 0x00,
	// 1, 0x0e,
	// 1, 0x14,
	// 1, 0x03,
	// 1, 0x11,
	// 1, 0x07,
	// 1, 0x31,
	// 1, 0xc1,
	// 1, 0x48,
	// 1, 0x08,
	// 1, 0x0f,
	// 1, 0x0c,
	// 1, 0x31,
	// 1, 0x36,
	// 1, 0x0f,
	// 0, 0x11, // Sleep Out
	// 0, 0x29, // Display ON
	// 0, 0x36, // Memory Access Control
	// 1, 0x08, //   Select BGR color filter
	// 0, 0x2a, // Column Address Set
	// 1, 0x00, //   Start column [15:8]
	// 1, 0x00, //   Start column [7:0]
	// 1, 0x00, //   End column [15:8]
	// 1, 0xef, //   End column [7:0]
	// 0, 0x2b, // Page Address Set
	// 1, 0x00, //   Start position [15:8]
	// 1, 0x00, //   Start position [7:0]
	// 1, 0x01, //   End position   [15:8]
	// 1, 0x3f, //   End position   [7:0]
	// 0, 0x35, // Tearing Effect Line ON
	// 0, 0x00, //   V-Blanking information only (set to 01h for both V-Blanking and H-Blanking information
	// 0, 0x2c, // Memory Write
	/* ==== End marker ==== */
	// 0xfe, 0xca,
	// 0x80, 0x00,
	// 0x00, 0x80,
	0x80, 0x80,
	// 0x40, 0x00,
	0,0,
	// 0x20, 0x20,
	// 0x10, 0x10,
};
static size_t lcd_init_rom_len = sizeof(lcd_init_rom) / sizeof(uint8_t);

static const char *TAG = "fpga-tests";

void set_parallel_mode(bool yes) {
	// uint8_t value_of = yes;
	// i2c_write_reg_n(RP2040_I2C_BUS, RP2040_I2C_ADDR, RP2040_REG_LCDM, &value_of, 1);
	// size_t limit = esp_timer_get_time() + 100000;
	// value_of = !yes;
	// while (value_of != yes && esp_timer_get_time() < limit) {
	// 	i2c_read_reg(RP2040_I2C_BUS, RP2040_I2C_ADDR, RP2040_REG_LCDM, &value_of, 1);
	// }
}

static void fpga_do_lcd() {
	
	ili9341_deinit(&display);
	set_parallel_mode(true);
	
	// Flash bitstream.
	esp_err_t res = ice40_load_bitstream(&dev_ice40, fpga_test_bin, fpga_test_bin_len);
	if (res != ESP_OK) {
		ESP_LOGE(TAG, "Writing bitstream failed.");
		vTaskDelay(3000 / portTICK_PERIOD_MS);
		esp_restart();
	} else {
		ESP_LOGI(TAG, "Bitstream written.");
	}
	
	// Dump in the LCD init ROM.
	uint8_t dummy[lcd_init_rom_len];
	vTaskDelay(5 / portTICK_PERIOD_MS);
	res = ice40_transaction(&dev_ice40, lcd_init_rom, lcd_init_rom_len, dummy, lcd_init_rom_len);
	if (res != ESP_OK) {
		ESP_LOGE(TAG, "Writing init ROM failed.");
		vTaskDelay(3000 / portTICK_PERIOD_MS);
		esp_restart();
	} else {
		ESP_LOGI(TAG, "Init ROM written.");
	}
	
	while (1) {
		// Limit of loop.
		bool exuent = false;
		rp2040_read_buttons(&dev_rp2040, &button_bits);
		exuent = !!(button_bits & MASK_BTN_HOME);
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
	rp2040_read_buttons(&dev_rp2040, &button_bits);
	a_button = !!(button_bits & MASK_BTN_ACCEPT);
	b_button = !!(button_bits & MASK_BTN_BACK);
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
		rp2040_read_buttons(&dev_rp2040, &button_bits);
		new_a_button = !!(button_bits & MASK_BTN_ACCEPT);
		new_b_button = !!(button_bits & MASK_BTN_BACK);
		
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
		rp2040_read_buttons(&dev_rp2040, &button_bits);
		exuent = !!(button_bits & MASK_BTN_HOME);
		if (exuent) break;
	}
}

void fpga_tests() {
	
	fpga_do_lcd();
	
}
