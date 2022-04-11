
#include "sample_menu.h"
#include "pax_gfx.h"

#include <esp_timer.h>

void menu_render(pax_buf_t *buf, menu_t *menu, int selected, pax_col_t color, float x, float y, float width, float height) {
    // Check how many entries fit.
    float  scroll       = 0;
    float  entry_height = menu->font_size + 2;
    int    n_fit        = buf->height / entry_height;
    
    // Simplify scroll variable.
    size_t entry_offset = scroll / entry_height;
    scroll -= entry_offset * entry_height;
    selected %= menu->n_entries;
    
    // Draw the entries.
    y -= scroll;
    pax_simple_line(buf, color, x + width - 1, y, x + width - 1, y + height - 1);
    pax_clip(buf, x, y, width, height);
    for (size_t i = entry_offset; i < menu->n_entries && i < entry_offset + n_fit; i++) {
        if (i == selected) {
            pax_simple_rect(buf, color, x, y, width - 1, entry_height);
            pax_clip(buf, 1, y + 1, width - 2, menu->font_size);
            pax_draw_text(buf, 0xff000000, NULL, menu->font_size, 1, y + 1, menu->entries[i].text);
            pax_clip(buf, x, y, width, height);
        } else {
            pax_simple_rect(buf, 0xff000000, x, y, width - 1, entry_height);
            pax_clip(buf, 1, y + 1, width - 2, menu->font_size);
            pax_draw_text(buf, color, NULL, menu->font_size, 1, y + 1, menu->entries[i].text);
            pax_clip(buf, x, y, width, height);
        }
        y += entry_height;
    }
    pax_noclip(buf);
}
