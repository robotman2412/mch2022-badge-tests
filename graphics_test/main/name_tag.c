
#include "include/main.h"

void name_tag() {
	pax_font_t *font  = pax_get_font("saira condensed");
	pax_font_t *lfont = pax_get_font("saira regular");
	
	// Determine text scale and co-ordinates.
	char      *name       = "Robot";
	char      *lname      = "RobotMan2412";
	pax_vec1_t name_size  = pax_text_size(font, 1,                  name);
	pax_vec1_t lname_size = pax_text_size(lfont, lfont->default_size, lname);
	float      name_scale = buf.width / name_size.x;
	name_size             = pax_text_size(font, name_scale,         name);
	float      name_y     = (buf.height - name_size.y) * 0.5;
	float      lname_x    = (buf.width - lname_size.x) * 0.5;
	float      lname_y    = buf.height - lname_size.y;
	
	// Await.
	while (1) {
		// PUT TEXT.
		uint64_t millis = esp_timer_get_time() / 1000;
		pax_col_t color = pax_col_hsv(millis * 255 / 8000, 255, 255);
		pax_background(&buf, 0);
		pax_draw_text_aa(&buf, color, font, name_scale, 0, name_y, name);
		pax_draw_text_aa(&buf, color, lfont, lfont->default_size, lname_x, lname_y, lname);
		
		// Barf it out.
		ili9341_write(&display, framebuffer);
		
		// Limit of loop.
		bool exuent = false;
		rp2040_read_buttons(&dev_rp2040, &button_bits);
		exuent = !!(button_bits & MASK_BTN_HOME);
		if (exuent) break;
	}
}
