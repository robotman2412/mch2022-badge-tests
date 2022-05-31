
#include "include/main.h"
#include <pax_codecs.h>

static const char *TAG = "png-tests";

#include "mch2022-demo/images/mch2021_half.c"


// extern char temp_logo_png[];
// extern size_t temp_logo_png_len;
// #include "../components/pax-codecs/codec-test-images/png_test.c"

void png_tests() {
	// pax_buf_t mypng;
	// if (pax_decode_png_buf(&mypng, mch2021_small_png, mch2021_small_png_len, PAX_BUF_4_PAL, CODEC_FLAG_OPTIMAL)) {
	// 	ESP_LOGI(TAG, "PNG decd success.");
	// 	pax_background(&buf, 0);
	// 	// pax_shade_rect(&buf, -1, &PAX_SHADER_TEXTURE(&mypng), NULL, 0, 0, 319, 241);
	// 	pax_draw_image(&buf, &mypng, 0, 0);
	// 	pax_buf_destroy(&mypng);
	// } else {
	// 	ESP_LOGE(TAG, "PNG decd failure.");
	// 	pax_background(&buf, 0xffff0000);
	// }
	
	if (pax_decode_png_buf(&buf, mch2021_half_png, mch2021_half_png_len, buf.type, CODEC_FLAG_EXISTING)) {
		ESP_LOGI(TAG, "PNG decd success.");
	} else {
		ESP_LOGE(TAG, "PNG decd failure.");
		pax_background(&buf, 0xffff0000);
	}
	
    // El flush.
    ili9341_write(&display, framebuffer);
    
	while (1) {
		// Limit of loop.
		bool exuent = false;
		rp2040_read_buttons(&dev_rp2040, &button_bits);
		exuent = !!(button_bits & MASK_BTN_HOME);
		if (exuent) break;
	}
	
}
