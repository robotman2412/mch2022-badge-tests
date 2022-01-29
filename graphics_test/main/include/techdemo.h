
#ifndef TECHDEMO_H
#define TECHDEMO_H

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "pax_gfx.h"

/* ================ types ================= */
#define TD_DRAW_NONE    0
#define TD_DRAW_SHAPES  1
#define TD_DRAW_SHIMMER 2
#define TD_DRAW_TRIS    3

#define TD_INTERP_TYPE_INT   0
#define TD_INTERP_TYPE_COL   1
#define TD_INTERP_TYPE_HSV   2
#define TD_INTERP_TYPE_FLOAT 3
#define TD_LINEAR 0
#define TD_EASE 1
#define TD_EASE_IN 2
#define TD_EASE_OUT 3

#define PAX_TD_BUF_TYPE  PAX_BUF_2_PAL
#define PAX_TD_BUF_STR  "PAX_BUF_2_PAL"

struct td_event;
struct td_lerp;
struct td_set;

typedef struct td_event td_event_t;
typedef struct td_lerp  td_lerp_t;
typedef struct td_set td_set_t;
typedef void (*td_func_t)(size_t planned_time, size_t planned_duration, void *args);

struct td_event {
    size_t     duration;
    td_func_t  callback;
    void      *callback_args;
};

struct td_lerp {
    td_lerp_t *prev;
    td_lerp_t *next;
    size_t     start;
    size_t     end;
    size_t     duration;
    union {
        int   *int_ptr;
        float *float_ptr;
    };
    union {
        int    int_from;
        float  float_from;
    };
    union {
        int    int_to;
        float  float_to;
    };
    uint8_t    type;
    uint8_t    timing;
};

struct td_set {
    size_t       size;
    void        *pointer;
    union {
        uint64_t value;
        float    f_value;
    };
};

/* ============== functions =============== */

// Initialise the tech demo.
// Framebuffer can be a initialised buffer of any type (color is recommended).
// Clipbuffer should be a PAX_TD_BUF_TYPE initialised buffer.
void pax_techdemo_init(pax_buf_t *framebuffer, pax_buf_t *clipbuffer);

// Draws the appropriate frame of the tech demo for the given time.
// Time is in milliseconds after the first frame.
// Returns true when running, false when finished.
bool pax_techdemo_draw(size_t millis);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //TECHDEMO_H
