
#include "include/main.h"
#include "pax_clock.h"

static const char *TAG = "THA CLOCK!";

void tha_clock() {
	pax_clock_ctx_t cctx;
	paxc_7seg(&buf, &cctx, 0);
	
	while (1) {
		
		if (ili9341_write(&display, framebuffer)) {
			ESP_LOGE(TAG, "Display write failed.");
			vTaskDelay(3000 / portTICK_PERIOD_MS);
			esp_restart();
		}
		taskYIELD();
		
		// Limit of loop.
		bool exuent = false;
		rp2040_read_buttons(&dev_rp2040, &button_bits);
		exuent = !!(button_bits & MASK_BTN_HOME);
		if (exuent) break;
	}
}
