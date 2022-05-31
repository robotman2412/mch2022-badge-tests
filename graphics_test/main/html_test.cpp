
#include "main.h"
#include "ph_compositor.hpp"

static const char *TAG = "html-test";

void html_test() {
    pax_push_2d(&buf);
    pax_reset_2d(&buf, PAX_RESET_TOP);
    pax_background(&buf, -1);
    
    phtml::Element *top = new phtml::Element();
    phtml::Text    *t0  = new phtml::Text("Text 0");
    phtml::Text    *t1  = new phtml::Text("Text 1\nnewline");
    phtml::Text    *t2  = new phtml::Text("Text 2");
    ESP_LOGW(TAG, "Append 0");
    top->appendChild(t0);
    ESP_LOGW(TAG, "Append 1");
    top->appendChild(t1);
    ESP_LOGW(TAG, "Append 2");
    top->appendChild(t2);
    ESP_LOGW(TAG, "Render");
    top->render(&buf, true);
    
    // El flush.
    ili9341_write(&display, framebuffer);
    
    while (1) {
        // Limit of loop.
        bool exuent = false;
		rp2040_read_buttons(&dev_rp2040, &button_bits);
        exuent = !!(button_bits & MASK_BTN_HOME);
        if (exuent) break;
	}
    
    pax_pop_2d(&buf);
}
