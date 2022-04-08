
#include "main.h"
#include <mch2022_keyb.h>

void gimmekeyboard() {
	char *val = mch_keyb_simple(&buf, &display, &dev_pca9555, NULL);
	if (val) free(val);
}

// #include <pax_keyboard.h>

// void gimmekeyboard() {
// 	pkb_ctx_t kb_ctx = {
// 		.content        = strdup(""),
// 		.content_cap    = 10,
// 		.kb_font        = pax_get_font("7x9"),
// 		.kb_font_size   = 3*9,
// 		.text_font      = pax_get_font("7x9"),
// 		.text_font_size = 2*9,
// 		.x              = 0,
// 		.y              = 0,
// 		.width          = buf.width,
// 		.height         = buf.height,
// 		.key_x          = 3,
// 		.key_y          = 1,
// 		.board_sel      = PKB_LOWERCASE,
// 		.dirty          = false,
// 		.kb_dirty       = false,
// 		.text_dirty     = false,
// 		.bg_col         = 0xff000000,
// 		.text_col       = 0xffff0000,
// 		.sel_col        = 0xffff0000,
// 		.sel_text_col   = 0xff000000,
// 	};
	
// 	bool last_up    = false;
// 	bool last_down  = false;
// 	bool last_left  = false;
// 	bool last_right = false;
// 	bool last_shift = false;
// 	bool last_acc   = true;
// 	bool last_back  = false;
// 	bool last_sel   = false;
	
// 	pkb_render(&buf, &kb_ctx);
// 	ili9341_write(&display, framebuffer);
	
// 	while (1) {
// 		bool up    = last_up;
// 		bool down  = last_down;
// 		bool left  = last_left;
// 		bool right = last_right;
// 		bool shift = last_shift;
// 		bool acc   = last_acc;
// 		bool back  = last_back;
// 		bool sel   = last_sel;
// 		pca9555_get_gpio_value(&dev_pca9555, PCA9555_PIN_BTN_JOY_UP,    &up);
// 		pca9555_get_gpio_value(&dev_pca9555, PCA9555_PIN_BTN_JOY_DOWN,  &down);
// 		pca9555_get_gpio_value(&dev_pca9555, PCA9555_PIN_BTN_JOY_LEFT,  &left);
// 		pca9555_get_gpio_value(&dev_pca9555, PCA9555_PIN_BTN_JOY_RIGHT, &right);
// 		pca9555_get_gpio_value(&dev_pca9555, PCA9555_PIN_BTN_JOY_PRESS, &shift);
// 		pca9555_get_gpio_value(&dev_pca9555, PCA9555_PIN_BTN_ACCEPT,    &acc);
// 		pca9555_get_gpio_value(&dev_pca9555, PCA9555_PIN_BTN_BACK,      &back);
// 		pca9555_get_gpio_value(&dev_pca9555, PCA9555_PIN_BTN_SELECT,    &sel);
		
// 		if (!last_up    && up)    pkb_press(&kb_ctx, PKB_UP);
// 		if (!last_down  && down)  pkb_press(&kb_ctx, PKB_DOWN);
// 		if (!last_left  && left)  pkb_press(&kb_ctx, PKB_LEFT);
// 		if (!last_right && right) pkb_press(&kb_ctx, PKB_RIGHT);
// 		if (!last_shift && shift) pkb_press(&kb_ctx, PKB_SHIFT);
// 		if (!last_acc   && acc)   pkb_press(&kb_ctx, PKB_CHARSELECT);
// 		if (!last_back  && back)  pkb_press(&kb_ctx, PKB_DELETE_BEFORE);
// 		if (!last_sel   && sel)   pkb_press(&kb_ctx, PKB_MODESELECT);
// 		if (last_up    && !up)    pkb_release(&kb_ctx, PKB_UP);
// 		if (last_down  && !down)  pkb_release(&kb_ctx, PKB_DOWN);
// 		if (last_left  && !left)  pkb_release(&kb_ctx, PKB_LEFT);
// 		if (last_right && !right) pkb_release(&kb_ctx, PKB_RIGHT);
// 		if (last_shift && !shift) pkb_release(&kb_ctx, PKB_SHIFT);
// 		if (last_acc   && !acc)   pkb_release(&kb_ctx, PKB_CHARSELECT);
// 		if (last_back  && !back)  pkb_release(&kb_ctx, PKB_DELETE_BEFORE);
// 		if (last_sel   && !sel)   pkb_release(&kb_ctx, PKB_MODESELECT);
// 		pkb_loop(&kb_ctx);
// 		if (kb_ctx.dirty) {
// 			pkb_redraw(&buf, &kb_ctx);
// 			ili9341_write(&display, framebuffer);
// 		}
		
// 		last_up    = up;
// 		last_down  = down;
// 		last_left  = left;
// 		last_right = right;
// 		last_shift = shift;
// 		last_acc   = acc;
// 		last_back  = back;
// 		last_sel   = sel;
		
// 		// Limit of loop.
// 		bool exuent = false;
// 		pca9555_get_gpio_value(&dev_pca9555, PCA9555_PIN_BTN_HOME, &exuent);
// 		if (exuent || (kb_ctx.input_accepted && !acc)) break;
// 	}
	
// 	free(kb_ctx.content);
// }
