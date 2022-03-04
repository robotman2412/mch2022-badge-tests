
#include "include/main.h"

static const char *TAG = "timer";

// Draws a band for the countdown.
// Offset and fill range from 0 to 1.
void draw_band(pax_col_t color, float inner_radius, float outer_radius, float fill, float offset) {
	// Grab an ARC.
	const size_t n_points = 2 + (size_t) (fill * 62);
	pax_vec1_t points[n_points];
	float a0 = offset * -2 * M_PI + M_PI * 0.5;
	float a1 = a0 - 2 * M_PI * fill;
	// float a0 = M_PI * 0.25f;
	// float a1 = M_PI * 1.25f;
	pax_vectorise_arc(points, n_points, 0, 0, 1, a0, a1);
	
	// Fart it out.
	for (size_t i = 0; i < n_points - 1; i++) {
		pax_draw_tri(
			&buf, color,
			points[i  ].x * inner_radius, points[i  ].y * inner_radius,
			points[i  ].x * outer_radius, points[i  ].y * outer_radius,
			points[i+1].x * outer_radius, points[i+1].y * outer_radius
		);
		pax_draw_tri(
			&buf, color,
			points[i  ].x * inner_radius, points[i  ].y * inner_radius,
			points[i+1].x * inner_radius, points[i+1].y * inner_radius,
			points[i+1].x * outer_radius, points[i+1].y * outer_radius
		);
	}
}

void countdown() {
	float ring_scale       = fminf(buf.width, buf.height) * 0.4f;
	float remain_scale     =  9;
	float title_scale      = 18;
	const char *remain_str = "1 day\nand 4 hours\nand 56 minutes\nand 1 second\nremains";
	const char *title_str  = "Until your mom";
	while (1) {
		pax_background(&buf, 0);
		uint64_t millis = esp_timer_get_time() / 1000;
		float    angle0 = ((millis >> 2) & 1023) / 1024.0f;
		float    angle1 = ((millis >> 3) & 1023) / 1024.0f;
		
		// Divide in to 6 parts (0.16), seconds gets 3 of them.
		// Slice off 1/5 of that which is 1/30 (0.03).
		const float dist3  = 1.0;
		const float dist2  = 5.0/ 6.0;
		const float dist1  = 4.0/ 6.0;
		const float dist0  = 3.0/ 6.0;
		const float margin = 1.0/30.0;
		
		pax_push_2d(&buf);
			pax_apply_2d(&buf, matrix_2d_translate(buf.width * 0.5, buf.height * 0.5));
			pax_apply_2d(&buf, matrix_2d_scale(ring_scale, ring_scale));
			// Days ring.
			draw_band(0xffff0000, dist2 + margin, dist3, angle0, angle1);
			// Hours ring.
			draw_band(0xffff0000, dist1 + margin, dist2, angle0, angle1);
			// Minutes ring.
			draw_band(0xffff0000, dist0 + margin, dist1, angle0, angle1);
			// Seconds ring.
			draw_band(0xffff0000, 0,              dist0, angle0, angle1);
		pax_pop_2d(&buf);
		
		pax_vec1_t remain_size = pax_text_size(NULL, remain_scale, remain_str);
		pax_draw_text(&buf, -1, NULL, remain_scale, 0, buf.height - remain_size.y, remain_str);
		pax_draw_text(&buf, -1, NULL, title_scale, 0, 0, title_str);
		
		if (ili9341_write(&display, framebuffer)) {
			ESP_LOGE(TAG, "Display write failed.");
			vTaskDelay(3000 / portTICK_PERIOD_MS);
			esp_restart();
		}
		
		// Limit of loop.
		bool exuent = false;
		pca9555_get_gpio_value(&dev_pca9555, PCA9555_PIN_BTN_HOME, &exuent);
		if (exuent) break;
	}
	
}
