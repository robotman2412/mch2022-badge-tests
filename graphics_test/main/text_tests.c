
#include "include/main.h"

void text_tests() {
	const pax_font_t *font = pax_get_font("saira condensed");
	const char *text = "Ye!";
	
	float y      = 0;
	float pt     = 15;
	float pt_inc = 10;
	int   num    = 6;
	
	pax_background(&buf, 0);
	for (int i = 0; i < num; i++, pt += pt_inc) {
		                  pax_draw_text   (&buf, -1, font, pt, 0,           y, text);
		pax_vec1_t dims = pax_draw_text_aa(&buf, -1, font, pt, buf.width/2, y, text);
		y += dims.y;
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
