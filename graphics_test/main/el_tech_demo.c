
#include "include/main.h"
#include "include/techdemo.h"

static const char *TAG = "tech-demo";

void el_tech_demo() {
	
	pax_techdemo_init(&buf, &clip);
	uint64_t start = esp_timer_get_time() / 1000;
	uint64_t last_time = start;
	char text_buf[32];
	while (1) {
		uint64_t now = esp_timer_get_time() / 1000 - start;
		bool fin = pax_techdemo_draw(now);
		uint64_t post = esp_timer_get_time() / 1000 - start;
		int fps_full = 1000 / (now - last_time);
		int fps_render = 1000 / (post - now);
		snprintf(text_buf, 31, "%d/%d FPS", fps_full, fps_render);
		pax_vec1_t text_size = pax_text_size(PAX_FONT_DEFAULT, 9, text_buf);
		pax_draw_text(&buf, -1, PAX_FONT_DEFAULT, 9, buf.width - text_size.x - 1, 0, text_buf);
		
		if (ili9341_write(&display, framebuffer)) {
			ESP_LOGE(TAG, "Display write failed.");
			vTaskDelay(3000 / portTICK_PERIOD_MS);
			esp_restart();
		}
		if (fin) {
			pax_techdemo_init(&buf, &clip);
			start = esp_timer_get_time() / 1000;
		}
		
		last_time = now;
		
		// Limit of loop.
		bool exuent = false;
		pca9555_get_gpio_value(&dev_pca9555, PCA9555_PIN_BTN_HOME, &exuent);
		if (exuent) break;
	}
}
