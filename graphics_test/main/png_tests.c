
#include "include/main.h"
#include <fake_file.h>
#include <pax_codecs.h>

static const char *TAG = "png-tests";

#include "mch2022-demo/images/mch2021_small.c"

void png_tests() {
	pax_buf_t mypng;
	if (pax_decode_png_buf(&mypng, mch2021_small_png, mch2021_small_png_len, PAX_BUF_8_PAL)) {
		ESP_LOGI(TAG, "PNG decd success.");
		pax_background(&buf, 0);
		pax_shade_rect(&buf, -1, &PAX_SHADER_TEXTURE(&mypng), NULL, 0, 0, mypng.width, mypng.height);
	} else {
		ESP_LOGE(TAG, "PNG decd failure.");
		pax_background(&buf, 0xffff0000);
	}
	
    // El flush.
    ili9341_write(&display, framebuffer);
    
	while (1) {
		// Limit of loop.
		bool exuent = false;
		pca9555_get_gpio_value(&dev_pca9555, PCA9555_PIN_BTN_HOME, &exuent);
		if (exuent) break;
	}
	
}
