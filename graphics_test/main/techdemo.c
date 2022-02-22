
#include "techdemo.h"
#include "pax_shaders.h"
#include "pax_shapes.h"

#include <esp_system.h>
#include <esp_err.h>
#include <esp_log.h>
#include <string.h>

static const char *TAG = "pax-techdemo";

/* ======= choreographed varialbes ======== */

// Next event in the choreography.
static size_t current_event;

// Scaling applied to the clip buffer.
static float      clip_scaling;
// Panning (translation) applied to the clip buffer (in parts of width).
static float      clip_pan_x;
// Panning (translation) applied to the clip buffer (in parts of height).
static float      clip_pan_y;
// Whether to overlay the clip buffer.
static bool       overlay_clip;

// Color used for text overlay.
static pax_col_t  text_col;
// String used for text overlay.
static char      *text_str;
// Point size used for text overlay.
static float      text_size;

// Whether to show the three shapes.
static int        to_draw;
// Additional parameters for drawing.
static float      angle_0;
// Additional parameters for drawing.
static float      angle_1;
// Additional parameters for drawing.
static float      angle_2;
// Additional parameters for drawing.
static float      angle_3;
// Additional parameters for drawing.
static float      angle_4;
// Additional parameters for drawing.
static float      angle_5;

// Scaling applied to the buffer.
static float      buffer_scaling;
// Panning (translation) applied to the buffer (in parts of width).
static float      buffer_pan_x;
// Panning (translation) applied to the buffer (in parts of height).
static float      buffer_pan_y;
// Whether to apply the background color.
static bool       use_background;
// Background applied to the buffer.
static pax_col_t  background_color;

// Linked list of interpolations.
static td_lerp_t *lerps = NULL;

/* ================ config ================ */

// Framebuffer to use.
static pax_buf_t *buffer = NULL;
// Clip buffer complementary to the framebuffer.
static pax_buf_t *clip_buffer = NULL;

// Width of the frame.
static int width;
// Height of the frame.
static int height;

// Whether or not the tech demo was initialised.
static bool is_initialised = false;
// Whether or not the initialisation warning was given.
static bool warning_made = false;
// Last time passed to pax_techdemo_draw.
static size_t current_time;
// Planned time for the next event.
static size_t planned_time;
// Palette used for the clip buffer.
static pax_col_t palette[4] = {
	0xffffffff,
	0xffffffff,
	0xffffffff,
	0x00000000
};

// Initialise the tech demo.
// Framebuffer can be a initialised buffer of any type (color is recommended).
// Clipbuffer should be a PAX_BUF_1_GREY initialised buffer.
void pax_techdemo_init(pax_buf_t *framebuffer, pax_buf_t *clipbuffer) {
	if (!framebuffer) {
		ESP_LOGE(TAG, "`framebuffer` must not be NULL.");
	} else if (!clipbuffer) {
		ESP_LOGE(TAG, "`clipbuffer` must not be NULL.");
	} else if (framebuffer->width != clipbuffer->width || framebuffer->height != clipbuffer->height) {
		ESP_LOGW(TAG, "`clipbuffer` must be the same size as `framebuffer` (%dx%d vs %dx%d).",
			clipbuffer->width, clipbuffer->height, framebuffer->width, framebuffer->height
		);
	} else {
		// Sanity checks.
		if (!PAX_IS_COLOR(framebuffer->type)) {
			ESP_LOGW(TAG, "`framebuffer` should be initialised with color.");
		}
		if (clipbuffer->type != PAX_TD_BUF_TYPE) {
			ESP_LOGW(TAG, "`clipbuffer` should be initialised as PAX_TD_BUF_TYPE (" PAX_TD_BUF_STR ").");
		}
		// Palette checks.
		if (PAX_IS_PALETTE(clipbuffer->type)) {
			clipbuffer->pallette = palette;
			if (clipbuffer->type == PAX_BUF_1_PAL) {
				clipbuffer->pallette_size = 2;
			} else {
				clipbuffer->pallette_size = 4;
			}
		}
		// Copy it over.
		buffer      = framebuffer;
		clip_buffer = clipbuffer;
		width       = buffer->width;
		height      = buffer->height;
		is_initialised = true;
		
		// Reset interpolation list.
		while (lerps) {
			lerps->prev = NULL;
			td_lerp_t *tmp = lerps->next;
			lerps->next = NULL;
			lerps = tmp;
		}
		
		// Reset variables.
		current_event    = 0;
		planned_time     = 0;
		
		palette[0]       = 0xffffffff;
		palette[1]       = 0xffffffff;
		palette[2]       = 0xffffffff;
		palette[3]       = 0x00000000;
		
		clip_scaling     = 1;
		clip_pan_x       = 0;
		clip_pan_y       = 0;
		overlay_clip     = true;
		
		text_col         = 0xffffffff;
		text_str         = NULL;
		text_size        = 18;
		
		to_draw          = TD_DRAW_SHAPES;
		angle_0          = 0;
		angle_1          = 0;
		angle_2          = 0;
		angle_3          = 0;
		
		buffer_scaling   = 1;
		buffer_pan_x     = 0;
		buffer_pan_y     = 0;
		background_color = 0;
		use_background   = true;
		
		ESP_LOGI(TAG, "PAX tech demo initialised successfully.");
	}
}

/* =============== shaders ================ */

// A shimmery shader.
pax_col_t td_shader_shimmer(pax_col_t tint, int x, int y, float u, float v, void *_args) {
	// Manhattan distance from the top left corner.
	float dist  = u + v;
	float scale = 0.2;
	float phase = (1 + scale) * 2 * angle_0 - scale;
	float value = dist - phase;
	pax_col_t normal    = 0xfff9e82a;
	pax_col_t highlight = 0xffffffff;
	if (value <= 0 && value >= -scale) {
		int part = 255 * (value / scale);
		return pax_col_lerp(part, normal, highlight);
	} else if (value >= 0 && value <= scale) {
		int part = 255 * (value / -scale);
		return pax_col_lerp(part, normal, highlight);
	} else {
		return normal;
	}
}

/* ============== functions =============== */

// Draws some title text on the clip buffer.
// If there's multiple lines (split by '\n'), that's the subtitle.
// Text is horizontally as wide as possible and vertically centered.
static void td_draw_title(size_t planned_time, size_t planned_duration, void *args) {
	char *raw        = strdup((char *) args);
	char *index      = strchr(raw, '\n');
	char *title;
	char *subtitle;
	pax_buf_t *buf   = clip_buffer;
	pax_font_t *font = PAX_FONT_DEFAULT;
	
	// Split it up just a bit.
	pax_col_t col = 0xff000000;
	pax_background(buf, 1);
	if (index) {
		*index   = 0;
		title    = raw;
		subtitle = index + 1;
		// Title and subtitle.
		pax_vec1_t title_size    = pax_text_size(font, 1, title);
		pax_vec1_t subtitle_size = pax_text_size(font, 1, subtitle);
		float title_scale        = width / title_size.x;
		float subtitle_scale     = width / subtitle_size.x;
		float total_height       = title_scale + subtitle_size.y * subtitle_scale;
		float title_y            = (height - total_height) * 0.5;
		float subtitle_y         = title_y + title_scale;
		pax_draw_text(buf, col, font, title_scale,    0, title_y,    title);
		pax_draw_text(buf, col, font, subtitle_scale, 0, subtitle_y, subtitle);
	} else {
		title    = raw;
		// Just the title.
		pax_vec1_t title_size    = pax_text_size(font, 1, title);
		float title_scale        = width / title_size.x;
		float title_y            = (height - title_scale) * 0.5;
		pax_draw_text(buf, col, font, title_scale, 0, title_y, title);
	}
}

// Linearly interpolate a variable.
static void td_add_lerp(size_t planned_time, size_t planned_duration, void *args) {
	td_lerp_t *lerp = args;
	lerp->start = planned_time;
	lerp->end   = lerp->duration + planned_time;
	lerp->prev  = NULL;
	lerp->next  = lerps;
	if (lerps) {
		lerps->prev = lerp;
	}
	lerps = lerp;
}

// Perform aforementioned interpolation.
static void td_perform_lerp(td_lerp_t *lerp) {
	float part = (current_time - lerp->start) / (float) (lerp->end - lerp->start);
	if (current_time <= lerp->start) {
		// Clip to beginning.
		part = 0.0;
	} else if (current_time >= lerp->end) {
		// Clip to end.
		part = 1.0;
	} else {
		switch (lerp->timing) {
			case TD_EASE_OUT:
				// Ease-out:    y=-x²+2x
				part = -part*part + 2*part;
				break;
			case TD_EASE_IN:
				// Ease-in:     y=x²
				part *= part;
				break;
			case TD_EASE:
				// Ease-in-out: y=-2x³+3x²
				part = -2*part*part*part + 3*part*part;
				break;
		}
	}
	switch (lerp->type) {
		uint32_t bleh;
		case TD_INTERP_TYPE_INT:
			// Interpolate an integer.
			*lerp->int_ptr = lerp->int_from + (lerp->int_to - lerp->int_from) * part;
			break;
		case TD_INTERP_TYPE_COL:
			// Interpolate a (RGB) color.
			*lerp->int_ptr = pax_col_lerp(part*255, lerp->int_from, lerp->int_to);
			break;
		case TD_INTERP_TYPE_HSV:
			// Interpolate a (HSV) color.
			bleh = pax_col_lerp(part*255, lerp->int_from, lerp->int_to);
			*lerp->int_ptr = pax_col_ahsv(bleh >> 24, bleh >> 16, bleh >> 8, bleh);
			break;
		case TD_INTERP_TYPE_FLOAT:
			// Interpolate a float.
			*lerp->float_ptr = lerp->float_from + (lerp->float_to - lerp->float_from) * part;
			break;
	}
}

// Set a variable of a primitive type.
static void td_set_var(size_t planned_time, size_t planned_duration, void *args) {
	td_set_t *set = (td_set_t *) args;
	memcpy(set->pointer, (void *) &set->value, set->size);
}

// Set THE string.
static void td_set_str(size_t planned_time, size_t planned_duration, void *args) {
	text_str = (char *) args;
}

/* =============== drawing ================ */

// Draws a square, a circle and a triangle.
static void td_draw_shapes() {
	float scale = fminf(width * 0.2, height * 0.4);
	pax_col_t col = 0xffff0000;
	
	pax_apply_2d(buffer, matrix_2d_translate(width * 0.5, height * 0.5));
	pax_apply_2d(buffer, matrix_2d_rotate(angle_1));
	
	// The square.
	pax_push_2d(buffer);
	pax_apply_2d(buffer, matrix_2d_translate(width * -0.25, 0));
	pax_apply_2d(buffer, matrix_2d_rotate(angle_0));
	pax_apply_2d(buffer, matrix_2d_scale(scale, scale));
	pax_draw_rect(buffer, col, -0.5, -0.5, 1, 1);
	pax_pop_2d(buffer);
	
	// The circle.
	pax_draw_circle(buffer, col, 0, 0, scale * 0.5);
	
	// The triangle.
	float my_sin = 0.866, my_cos = 0.5;
	pax_apply_2d(buffer, matrix_2d_translate(width * 0.25, 0));
	pax_apply_2d(buffer, matrix_2d_rotate(angle_0));
	pax_apply_2d(buffer, matrix_2d_scale(scale * 0.6, scale * 0.6));
	pax_draw_tri(buffer, col, -my_cos, -my_sin, -my_cos, my_sin, 1, 0);
}

// Draws a funny shimmer.
static void td_draw_shimmer() {
	pax_apply_2d(buffer, matrix_2d_translate(width * 0.5, height * 0.5));
	pax_apply_2d(buffer, matrix_2d_rotate(angle_1));
	pax_shader_t shader = {
		.callback          = td_shader_shimmer,
		.alpha_promise_0   = false,
		.alpha_promise_255 = true
	};
	pax_shade_rect(buffer, -1, &shader, NULL, -50, -50, 100, 100);
}

// Show arcs and curves.
static void td_draw_curves() {
	// Bezier curve control points.
	pax_vec4_t ctl0 = {
		.x0 = buffer->width * 0.05,  .y0 = buffer->height * 0.5,
		.x1 = buffer->width * 0.15,  .y1 = buffer->height * 0.95,
		.x2 = buffer->width * 0.35,  .y2 = buffer->height * 0.5,
		.x3 = buffer->width * 0.5,   .y3 = buffer->height * 0.5
	};
	// Floating crap control points.
	const pax_vec1_t el_tri[] = {
		(pax_vec1_t) { .x =  0.25f,  .y =  0.0f    },
		(pax_vec1_t) { .x = -0.125f, .y =  0.2165f },
		(pax_vec1_t) { .x = -0.125f, .y = -0.2165f },
		(pax_vec1_t) { .x =  0.25f,  .y =  0.0f    },
	};
	const pax_vec1_t el_rect[] = {
		(pax_vec1_t) { .x = -0.25f, .y = -0.25f },
		(pax_vec1_t) { .x =  0.25f, .y = -0.25f },
		(pax_vec1_t) { .x =  0.25f, .y =  0.25f },
		(pax_vec1_t) { .x = -0.25f, .y =  0.25f },
		(pax_vec1_t) { .x = -0.25f, .y = -0.25f },
	};
	
	// Curve values.
	float bez0_from = fmaxf(0, fminf(angle_0,     1));
	float bez0_to   = fmaxf(0, fminf(angle_1,     1));
	float arc0_from = fmaxf(0, fminf(angle_0 - 1, 1));
	float arc0_to   = fmaxf(0, fminf(angle_1 - 1, 1));
	float crap_from = fmaxf(0, fminf(angle_4,     1));
	float crap_to   = fmaxf(0, fminf(angle_3,     1));
	
	// First curve.
	if (bez0_from != bez0_to) {
		pax_draw_bezier_part(buffer, -1, ctl0, bez0_from, bez0_to);
	}
	// Arc.
	if (arc0_from != arc0_to) {
		float a0   = arc0_from * M_PI * -0.25 + M_PI * 0.5;
		float a1   = arc0_to   * M_PI * -0.25 + M_PI * 0.5;
		float diff = arc0_to - arc0_from;
		int   bri  = diff * 255;
		float x    = buffer->width  * 0.5;
		float y    = buffer->height * 0.75;
		float r    = buffer->height * 0.25;
		
		pax_outline_arc(buffer, -1, x, y, r, a0, a1);
		
		pax_col_t stretch = pax_col_hsv(0, 0, bri);
		
		pax_push_2d(buffer);
		pax_apply_2d(buffer, matrix_2d_translate(x, y));
		
		pax_apply_2d(buffer, matrix_2d_rotate(a0));
		pax_draw_line(buffer, stretch, 0, 0, r, 0);
		
		pax_apply_2d(buffer, matrix_2d_rotate(a1 - a0));
		pax_draw_line(buffer, stretch, 0, 0, r, 0);
		
		pax_pop_2d(buffer);
	}
	// Floaty shapes.
	if (crap_from != crap_to) {
		pax_push_2d(buffer);
			pax_apply_2d(buffer, matrix_2d_translate(buffer->width  * 0.25,  buffer->height * 0.25));
			pax_apply_2d(buffer, matrix_2d_scale    (buffer->height * 0.25, buffer->height * 0.25));
			pax_apply_2d(buffer, matrix_2d_rotate   (angle_2));
			pax_outline_shape_part(buffer, -1, 4, el_tri, crap_from, crap_to);
		pax_pop_2d(buffer);
		pax_push_2d(buffer);
			pax_apply_2d(buffer, matrix_2d_translate(buffer->width  * 0.75,  buffer->height * 0.25));
			pax_apply_2d(buffer, matrix_2d_scale    (buffer->height * 0.25, buffer->height * 0.25));
			pax_apply_2d(buffer, matrix_2d_rotate   (angle_2));
			pax_outline_shape_part(buffer, -1, 5, el_rect, crap_from, crap_to);
		pax_pop_2d(buffer);
	}
}

/* ============= choreography ============= */
/*
	Goal:
	Show off PAX' features while hiding performance limitations.
	Features to show (in no particular order):
	  ✓ Triangles
	  - Arcs
	  ✓ Circles
	  - Clipping
	  ✓ Advanced shaders
	  - Texure mapping
	  - Curves
	Relevant notes:
	  - MCH2022 sponsors should probably go here, before the demo.
	  - There should be an always present "skip" option (except sponsors maybe).
*/

#define TD_DELAY(time) {.duration=time,.callback=NULL}
#define TD_DRAW_TITLE(title, subtitle) {.duration=0,.callback=td_draw_title,.callback_args=title"\n"subtitle}
#define TD_INTERP_INT(delay_time, interp_time, timing_func, variable, from, to) {\
			.duration = delay_time,\
			.callback = td_add_lerp,\
			.callback_args = &(td_lerp_t){\
				.duration = interp_time,\
				.int_ptr  = (int *) &(variable),\
				.int_from = (from),\
				.int_to   = (to),\
				.type     =  TD_INTERP_TYPE_INT,\
				.timing   =  timing_func\
			}\
		}
#define TD_INTERP_COL(delay_time, interp_time, timing_func, variable, from, to) {\
			.duration = delay_time,\
			.callback = td_add_lerp,\
			.callback_args = &(td_lerp_t){\
				.duration = interp_time,\
				.int_ptr  = (int *) &(variable),\
				.int_from = (from),\
				.int_to   = (to),\
				.type     =  TD_INTERP_TYPE_COL,\
				.timing   =  timing_func\
			}\
		}
#define TD_INTERP_AHSV(delay_time, interp_time, timing_func, variable, from, to) {\
			.duration = delay_time,\
			.callback = td_add_lerp,\
			.callback_args = &(td_lerp_t){\
				.duration = interp_time,\
				.int_ptr  = (int *) &(variable),\
				.int_from = (from),\
				.int_to   = (to),\
				.type     =  TD_INTERP_TYPE_HSV,\
				.timing   =  timing_func\
			}\
		}
#define TD_INTERP_FLOAT(delay_time, interp_time, timing_func, variable, from, to) {\
			.duration = delay_time,\
			.callback = td_add_lerp,\
			.callback_args = &(td_lerp_t){\
				.duration   = interp_time,\
				.float_ptr  = (float *) &(variable),\
				.float_from = (from),\
				.float_to   = (to),\
				.type       =  TD_INTERP_TYPE_FLOAT,\
				.timing     =  timing_func\
			}\
		}
#define TD_SET_0(type_size, variable, new_value) {\
			.duration = 0,\
			.callback = td_set_var,\
			.callback_args = &(td_set_t){\
				.size     = (type_size),\
				.pointer  = (void *) &(variable),\
				.value    = (new_value)\
			}\
		}
#define TD_SET_BOOL(variable, value) TD_SET_0(sizeof(bool), variable, value)
#define TD_SET_INT(variable, value) TD_SET_0(sizeof(int), variable, value)
#define TD_SET_LONG(variable, value) TD_SET_0(sizeof(long), variable, value)
#define TD_SET_FLOAT(variable, new_value) {\
			.duration = 0,\
			.callback = td_set_var,\
			.callback_args = &(td_set_t){\
				.size     = sizeof(float),\
				.pointer  = (void *) &(variable),\
				.f_value  = (new_value)\
			}\
		}
#define TD_SET_STR(value) {\
			.duration = 0,\
			.callback = td_set_str,\
			.callback_args = (value)\
		}
#define TD_SHOW_TEXT(str) \
		TD_SET_STR(str),\
		TD_INTERP_COL(0, 2500, TD_EASE_IN, text_col, 0xffffffff, 0x00ffffff)

static td_event_t events[] = {
	// Prerender some text.
	TD_DRAW_TITLE  ("MCH2022", "graphics tech demo"),
	
	// Fade out a cutout.
	TD_INTERP_COL  (1500, 1500, TD_LINEAR,  palette[0], 0xffffffff, 0),
	TD_INTERP_COL  (2400, 2400, TD_LINEAR,  palette[1], 0xffffffff, 0),
	TD_SET_BOOL    (overlay_clip, false),
	
	// Start spinning the shapes.
	TD_SHOW_TEXT   ("2D transformations"),
	TD_INTERP_FLOAT(2000, 4000, TD_EASE,     angle_0, 0, M_PI*3),
	TD_INTERP_FLOAT(2000, 4000, TD_EASE_IN,  angle_1, 0, M_PI*2),
	
	// Zoom in on the circle.
	TD_INTERP_FLOAT(   0, 2000, TD_EASE_IN,  buffer_scaling, 1, 3),
	TD_INTERP_COL  (2500, 2000, TD_EASE_IN,  background_color, 0, 0xffff0000),
	
	// Show the shimmer effect.
	TD_SHOW_TEXT   ("Shader support"),
	TD_SET_INT     (to_draw,    TD_DRAW_SHIMMER),
	TD_SET_BOOL    (use_background, false),
	TD_INTERP_FLOAT(   0,  500, TD_EASE_OUT, buffer_scaling, 0.00001, 1),
	TD_INTERP_FLOAT( 500,  500, TD_EASE,     angle_1, M_PI*0.5, 0),
	TD_INTERP_FLOAT(1500, 1500, TD_EASE,     angle_0, 0, 1),
	
	// Fade away the yelloughw.
	TD_SET_BOOL    (use_background, true),
	TD_INTERP_FLOAT(   0,  500, TD_EASE_OUT, buffer_scaling, 1, 0.00001),
	TD_INTERP_COL  ( 500,  500, TD_EASE,     background_color, 0xffff0000, 0xff000000),
	
	// Draw funny arcs and curves.
	TD_SHOW_TEXT   ("Curves"),
	TD_SET_INT     (to_draw,    TD_DRAW_CURVES),
	TD_SET_FLOAT   (buffer_scaling, 1),
	TD_SET_FLOAT   (angle_0,        0),
	TD_SET_FLOAT   (angle_4,        0),
	TD_INTERP_FLOAT(   0, 4500, TD_EASE,     angle_2, 0, M_PI * 0.5),
	TD_INTERP_FLOAT(   0, 3000, TD_EASE,     angle_3, 0, 1),
	TD_INTERP_FLOAT(1500, 1500, TD_EASE,     angle_1, 0, 1),
	TD_INTERP_FLOAT( 500,  500, TD_EASE,     angle_1, 1, 2),
	TD_INTERP_FLOAT(1000, 1000, TD_EASE,     angle_1, 2, 3),
	
	TD_INTERP_FLOAT(   0, 1500, TD_EASE,     angle_4, 0, 1),
	TD_INTERP_FLOAT( 750,  750, TD_EASE,     angle_0, 0, 1),
	TD_INTERP_FLOAT( 250,  250, TD_EASE,     angle_0, 1, 2),
	TD_INTERP_FLOAT( 500,  500, TD_EASE,     angle_0, 2, 3),
	
	// Become colors.
	TD_SHOW_TEXT   ("Colorful"),
	TD_SET_INT     (to_draw, TD_DRAW_NONE),
	TD_INTERP_AHSV (2000, 2000, TD_EASE_IN,  background_color, 0xff000000, 0xffffffff),
	TD_INTERP_AHSV (2000, 2000, TD_EASE_OUT, background_color, 0xff00ffff, 0xffff00ff),
	
	// Mark the end.
	TD_DELAY       (   0),
};
static size_t n_events = sizeof(events) / sizeof(td_event_t);

// Draws the appropriate frame of the tech demo for the given time.
// Time is in milliseconds after the first frame.
// Returns true when running, false when finished.
bool pax_techdemo_draw(size_t now) {
	if (!is_initialised) {
		if (!warning_made) {
			ESP_LOGE(TAG, "PAX tech demo was not initialised, call `pax_techdemo_init` first.");
			warning_made = true;
		}
		return true;
	}
	bool finished = false;
	current_time = now;
	
	// Perform interpolations.
	td_lerp_t *lerp = lerps;
	while (lerp) {
		bool remove = lerp->end <= current_time;
		if (remove) {
			// Perform the final frame before removing it.
			td_perform_lerp(lerp);
			// Unlink it.
			if (lerp->prev) lerp->prev->next = lerp->next;
			else lerps = lerp->next;
			if (lerp->next) lerp->next->prev = lerp->prev;
		}
		lerp = lerp->next;
	}
	
	// Handle events.
	if (current_event < n_events) {
		while (current_event < n_events && planned_time <= now) {
			td_event_t event = events[current_event];
			if (event.callback) {
				ESP_LOGI(TAG, "Performing event %d.", current_event);
				event.callback(planned_time, event.duration, event.callback_args);
			} else {
				ESP_LOGI(TAG, "Skipping event %d.", current_event);
			}
			planned_time += event.duration;
			current_event ++;
		}
	} else {
		finished = true;
	}
	
	// Perform interpolations.
	lerp = lerps;
	while (lerp) {
		td_perform_lerp(lerp);
		lerp = lerp->next;
	}
	
	// Use the dirty window to save resources.
	pax_background(buffer, background_color);
	
	pax_push_2d(buffer);
	// Apply transformations.
	pax_apply_2d(buffer, matrix_2d_translate(width * 0.5, height * 0.5));
	pax_apply_2d(buffer, matrix_2d_scale(buffer_scaling, buffer_scaling));
	pax_apply_2d(buffer, matrix_2d_translate(buffer_pan_x - width * 0.5, buffer_pan_y - height * 0.5));
	
	// Draw the current scene.
	switch (to_draw) {
		case TD_DRAW_SHAPES:
			td_draw_shapes();
			break;
		case TD_DRAW_SHIMMER:
			td_draw_shimmer();
			break;
		case TD_DRAW_CURVES:
			td_draw_curves();
			break;
	}
	
	pax_pop_2d(buffer);
	
	// Draw the text overlay.
	pax_col_t text_col_merged = pax_col_merge(background_color, text_col);
	if (text_col_merged != background_color) {
		pax_draw_text(buffer, text_col_merged, PAX_FONT_DEFAULT, text_size, 0, 0, text_str);
	}
	
	// Draw the global overlay.
	if (overlay_clip) {
		pax_push_2d(buffer);
		pax_apply_2d(buffer, matrix_2d_scale(clip_scaling, clip_scaling));
		pax_apply_2d(buffer, matrix_2d_translate(clip_pan_x * width, clip_pan_y * height));
		pax_shade_rect(buffer, -1, &PAX_SHADER_TEXTURE(clip_buffer), NULL, 0, 0, width, height);
		pax_pop_2d(buffer);
	}
	
	return finished;
}
