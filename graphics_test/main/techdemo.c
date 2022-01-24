
#include "techdemo.h"
#include "pax_shaders.h"

#include <esp_system.h>
#include <esp_err.h>
#include <esp_log.h>
#include <string.h>

static const char *TAG = "pax-techdemo";

/* ======= choreographed varialbes ======== */

// Next event in the choreography.
static size_t current_event = 0;

// Scaling applied to the clip buffer.
static float      clip_scaling = 1;
// Panning (translation) applied to the clip buffer (in parts of width).
static float      clip_pan_x = 0;
// Panning (translation) applied to the clip buffer (in parts of height).
static float      clip_pan_y = 0;
// Whether to overlay the clip buffer.
static bool       overlay_clip = true;

// Color used for text overlay.
static pax_col_t  text_col = 0x00ffffff;
// String used for text overlay.
static char      *text_str = NULL;
// Point size used for text overlay.
static float      text_size = 18;

// Whether to show the three shapes.
static uint8_t    to_draw = TD_DRAW_SHAPES;
// Angle for rotating shapes.
static float      angle_0 = 0;
// Angle for orbiting shapes.
static float      angle_1 = 0;

// Scaling applied to the buffer.
static float      buffer_scaling = 1;
// Panning (translation) applied to the buffer (in parts of width).
static float      buffer_pan_x = 0;
// Panning (translation) applied to the buffer (in parts of height).
static float      buffer_pan_y = 0;
// Background applied to the buffer.
static pax_col_t  background_color = 0;

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
		
		// Reset variables.
		current_event    = 0;
		
		palette[0]       = 0xffffffff;
		palette[1]       = 0xffffffff;
		palette[2]       = 0xffffffff;
		palette[3]       = 0x00000000;
		
		clip_scaling     = 1;
		clip_pan_x       = 0;
		clip_pan_y       = 0;
		overlay_clip     = true;
		
		text_col         = 0x00ffffff;
		text_str         = NULL;
		text_size        = 18;
		
		to_draw          = TD_DRAW_SHAPES;
		angle_0          = 0;
		angle_1          = 0;
		
		buffer_scaling   = 1;
		buffer_pan_x     = 0;
		buffer_pan_y     = 0;
		background_color = 0;
		
		ESP_LOGI(TAG, "PAX tech demo initialised successfully.");
	}
}

/* ============== functions =============== */

// Draws some title text on the clip buffer.
// If there's multiple lines (split by '\n'), that's the subtitle.
// Text is horizontally as wide as possible and vertically centered.
static void td_draw_title(void *args) {
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

static td_lerp_t *lerps = NULL;

// Linearly interpolate a variable.
static void td_add_lerp(void *args) {
	td_lerp_t *lerp = args;
	lerp->prev = NULL;
	lerp->next = lerps;
	if (lerps) {
		lerps->prev = lerp;
	}
	lerps = lerp;
}

// Perform aforementioned interpolation.
static bool td_perform_lerp(td_lerp_t *lerp) {
	float part = (current_time - lerp->start) / (float) (lerp->end - lerp->start);
	if (current_time >= lerp->end) {
		part = 1.0;
	} else {
		switch (lerp->timing) {
			case TD_EASE_OUT:
				part = sin(M_PI * 0.5 * part);
				break;
			case TD_EASE_IN:
				part = 1 - sin(M_PI * 0.5 * (part + 1));
				break;
			case TD_EASE:
				part = 0.5 - 0.5 * cos(M_PI * part);
				break;
		}
	}
	switch (lerp->type) {
		case TD_INTERP_TYPE_INT:
			*lerp->int_ptr = lerp->int_from + (lerp->int_to - lerp->int_from) * part;
			break;
		case TD_INTERP_TYPE_COL:
			*lerp->int_ptr = pax_col_lerp(part*255, lerp->int_from, lerp->int_to);
			break;
		case TD_INTERP_TYPE_FLOAT:
			*lerp->float_ptr = lerp->float_from + (lerp->float_to - lerp->float_from) * part;
			break;
	}
	return current_time >= lerp->end;
}

// Set a variable of a primitive type.
static void td_set_var(void *args) {
	td_set_t *set = (td_set_t *) args;
	memcpy(set->pointer, &set->value, set->size);
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

// Draws a funny spedometer.
static void td_draw_speed() {
	
}

/* ============= choreography ============= */
/*
	Goal:
	Show off PAX' features while hiding performance limitations.
	Features to show (in no particular order):
	  - Triangles
	  - Arcs and circles
	  - Clipping
	  - Advanced shaders
	  - Texure mapping
	Relevant notes:
	  - MCH2022 sponsors should probably go here, before the demo.
	  - There should be an always present "skip" option (except sponsors maybe).
	
	Visual timeline:
	Start   End     Event
	0s      1.5s    Fade from white to a trasparent title text.
	1.5s    4s      Fade away title text, revealing the animation below.
	
	4s				Start spinning a triangle, a circle and a square on the horizontal center line.
	5s      8s      Show text stating something about optimisations.
	
	Technical timeline:
	Time        Event
	000.000     Render title "MCH2022", subtitle "gfx tech demo" on the clip buffer.
	  0.000     Fade palette color 0: 0s (white) -> 1.5s (clip).
	  1.500     Fade palette color 1: 1.5s (clip) -> 3.9s (fade in).
	  3.900     Remove the overlay text.
	  4.000     Start spinning the shapes.
	  6.000     Start orbiting the shapes.
	  8.000     Start zooming in on the circle.
	  8.000     Fade the background to red.
	 10.000     Demo ends.
*/

#define TD_DRAW_TITLE(title, subtitle) .callback=td_draw_title,.callback_args=title"\n"subtitle
#define TD_INTERP_INT(start_time, end_time, timing_func, variable, from, to) \
		.callback = td_add_lerp,\
		.callback_args = &(td_lerp_t){\
			.start    =  start_time,\
			.end      =  end_time,\
			.int_ptr  = (int *) &(variable),\
			.int_from = (from),\
			.int_to   = (to),\
			.type     =  TD_INTERP_TYPE_INT,\
			.timing   =  timing_func\
		}
#define TD_INTERP_COL(start_time, end_time, timing_func, variable, from, to) \
		.callback = td_add_lerp,\
		.callback_args = &(td_lerp_t){\
			.start    =  start_time,\
			.end      =  end_time,\
			.int_ptr  = (int *) &(variable),\
			.int_from = (from),\
			.int_to   = (to),\
			.type     =  TD_INTERP_TYPE_COL,\
			.timing   =  timing_func\
		}
#define TD_INTERP_FLOAT(start_time, end_time, timing_func, variable, from, to) \
		.callback = td_add_lerp,\
		.callback_args = &(td_lerp_t){\
			.start      =  start_time,\
			.end        =  end_time,\
			.float_ptr  = (float *) &(variable),\
			.float_from = (from),\
			.float_to   = (to),\
			.type       =  TD_INTERP_TYPE_FLOAT,\
			.timing     =  timing_func\
		}
#define TD_SET_0(type_size, variable, new_value) \
		.callback = td_set_var,\
		.callback_args = &(td_set_t){\
			.size    = (type_size),\
			.pointer = (void *) &(variable),\
			.value   = (uint64_t) (new_value)\
		}
#define TD_SET_BOOL(variable, value) TD_SET_0(sizeof(bool), variable, value)
#define TD_SET_STR(variable, value) TD_SET_0(sizeof(char *), variable, value)
#define TD_SET_INT(variable, value) TD_SET_0(sizeof(int), variable, value)
#define TD_SET_LONG(variable, value) TD_SET_0(sizeof(long), variable, value)
#define TD_SET_FLOAT(variable, new_value) \
		.callback = td_set_var,\
		.callback_args = &(td_set_t){\
			.size    = sizeof(float),\
			.pointer = (void *) &(variable),\
			.f_value = (new_value)\
		}

static td_event_t events[] = {
	{
		// Prerender some text.
		.time = 0,
		TD_DRAW_TITLE("MCH2022", "graphics tech demo")
	}, {
		// Fade out a cutout.
		.time = 0,
		TD_INTERP_COL(   0, 1500, TD_LINEAR, palette[0], 0xffffffff, 0)
	}, {
		// Fade out the remaining overlay.
		.time = 1500,
		TD_INTERP_COL(1500, 3900, TD_LINEAR, palette[1], 0xffffffff, 0)
	}, {
		// Remove the overlay.
		.time = 3900,
		TD_SET_BOOL(overlay_clip, false)
	}, {
		// Start spinning the shapes.
		.time = 4000,
		TD_INTERP_FLOAT(4000, 8000, TD_EASE, angle_0, 0, M_PI*3)
	}, {
		// Start orbits.
		.time = 6000,
		TD_INTERP_FLOAT(6000, 10000, TD_EASE_IN, angle_1, 0, M_PI*2)
	}, {
		// Zoom in on the circle.
		.time = 8000,
		TD_INTERP_FLOAT(8000, 10000, TD_EASE_IN, buffer_scaling, 1, 3)
	}, {
		// And fade the background to red.
		.time = 8000,
		TD_INTERP_COL(8000, 10000, TD_EASE_IN, background_color, 0, 0xffff0000)
	}, {
		// Final event: marks the end.
		.time = 10000,
		.callback = NULL
	}
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
	
	// Handle events.
	if (current_event < n_events) {
		while (current_event < n_events && events[current_event].time <= now) {
			td_event_t event = events[current_event];
			if (event.callback) {
				ESP_LOGI(TAG, "Performing event %d.", current_event);
				event.callback(event.callback_args);
			} else {
				ESP_LOGI(TAG, "Skipping event %d.", current_event);
			}
			current_event ++;
		}
	} else {
		finished = true;
	}
	
	// Perform interpolations.
	td_lerp_t *lerp = lerps;
	while (lerp) {
		bool remove = td_perform_lerp(lerp);
		if (remove) {
			if (lerp->prev) lerp->prev->next = lerp->next;
			else lerps = lerp->next;
			if (lerp->next) lerp->next->prev = lerp->prev;
		}
		lerp = lerp->next;
	}
	
	// Use the dirty window to save resources.
	pax_background(buffer, background_color);
	
	pax_push_2d(buffer);
	pax_apply_2d(buffer, matrix_2d_translate(width * 0.5, height * 0.5));
	pax_apply_2d(buffer, matrix_2d_scale(buffer_scaling, buffer_scaling));
	pax_apply_2d(buffer, matrix_2d_translate(buffer_pan_x - width * 0.5, buffer_pan_y - height * 0.5));
	
	// Spinny shapes.
	switch (to_draw) {
		case TD_DRAW_SHAPES:
			td_draw_shapes();
			break;
		case TD_DRAW_SPEED:
			td_draw_speed();
			break;
	}
	
	pax_pop_2d(buffer);
	
	// The funny text clippening.
	if (overlay_clip) {
		pax_push_2d(buffer);
		pax_apply_2d(buffer, matrix_2d_scale(clip_scaling, clip_scaling));
		pax_apply_2d(buffer, matrix_2d_translate(clip_pan_x * width, clip_pan_y * height));
		pax_shade_rect(buffer, -1, &PAX_SHADER_TEXTURE(clip_buffer), NULL, 0, 0, width-1, height-1);
		pax_pop_2d(buffer);
	}
	
	return finished;
}
