
#ifndef SAMPLE_MENU_H
#define SAMPLE_MENU_H

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "pax_gfx.h"

struct menu_entry;
struct menu;

typedef struct menu_entry menu_entry_t;
typedef struct menu       menu_t;

typedef bool (*menu_callback_t)();

struct menu_entry {
    char           *text;
    menu_callback_t callback;
    void           *callback_args;
};

struct menu {
    size_t          n_entries;
    menu_entry_t   *entries;
    char           *font;
    size_t          font_size;
};

void menu_render(pax_buf_t *buf, menu_t *menu, int selection, pax_col_t color, float x, float y, float width, float height);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //SAMPLE_MENU_H
