
#include "include/main.h"

static char *results = NULL;

static int64_t startTime     = 0;
static char   *currentThing  = NULL;
static int     currentAmount = 0;

void addResult(char *toAdd) {
	results = realloc(results, strlen(results) + strlen(toAdd) + 1);
	strcat(results, toAdd);
}

void showResults(char *extra) {
	if (!results) {
		results = malloc(1);
		*results = 0;
	}
	pax_vec1_t dims = pax_text_size(NULL, 9, results);
	pax_background(&buf, 0);
	pax_draw_text(&buf, -1, NULL, 9, 0, 0, results);
	if (extra) {
		pax_draw_text(&buf, 0xff00ff00, NULL, 9, 0, dims.y - 9, extra);
	}
	ili9341_write(&display, framebuffer);
}

void benchmarkpre(char *what, int amount) {
	if (currentThing) free(currentThing);
	currentThing = strdup(what);
	currentAmount = amount;
	
	char *buf = malloc(strlen(currentThing) + 60);
	sprintf(buf, "Benchmarking %dx %s", currentAmount, currentThing);
	showResults(buf);
	free(buf);
	
	startTime = esp_timer_get_time();
}

void benchmarkpost() {
	int64_t now   = esp_timer_get_time();
	if (!currentThing) return;
	int64_t took  = now - startTime;
	double perSec = currentAmount / (double) took * 1000000.0;
	
	char *buf = malloc(strlen(currentThing) + 60);
	sprintf(buf, "%s: %d in %lldms; %10.3lf/s\n", currentThing, currentAmount, took/1000, perSec);
	addResult(buf);
	ESP_LOGI("benchmark", "%s", buf);
	free(buf);
}

#define BENCHMARK(thing, times, code) \
	benchmarkpre(thing, times); \
	for (int i = times; i > 0; i --) { code; } \
	benchmarkpost();


// A shimmery shader.
pax_col_t testshader(pax_col_t tint, int x, int y, float u, float v, void *_args) {
	return tint;
}


void benchmark() {
	
	if (results) {
		free(results);
		results = NULL;
	}
	
	// Background fill benchmark.
	BENCHMARK("background", 200, {
		pax_background(&buf, 0);
	});
	
	// Screen write benchmark.
	BENCHMARK("ili9341_write", 100, {
		ili9341_write(&display, framebuffer);
	});
	
	// Matrix transform benchmark.
	matrix_2d_t mtx_a = matrix_2d_scale(50, 50);
	matrix_2d_t mtx_b = matrix_2d_rotate(1.2342114);
	BENCHMARK("matrix_mult", 200000, {
		matrix_2d_multiply(mtx_a, mtx_b);
	});
	
	// Circle benchmark.
	BENCHMARK("circle(r=100)", 200, {
		pax_draw_circle(&buf, -1, 100, 100, 100);
	});
	BENCHMARK("circle(r=10)", 2000, {
		pax_draw_circle(&buf, -1, 100, 100, 10);
	});
	
	// Rectangle benchmark.
	BENCHMARK("rect(100,100)", 200, {
		pax_draw_rect(&buf, -1, 0, 0, 100, 100);
	});
	BENCHMARK("rect(10,10)", 1000, {
		pax_draw_rect(&buf, -1, 0, 0, 10, 10);
	});
	
	// Shaded triangle benchmark.
	pax_shader_t shader = {
		.callback          = testshader,
		.callback_args     = NULL,
		.alpha_promise_0   = false,
		.alpha_promise_255 = false,
	};
	BENCHMARK("shade(a=5000)", 200, {
		pax_shade_tri(
			&buf, -1, &shader, NULL,
			0,   0,
			100, 0,
			0,   100
		);
	});
	BENCHMARK("shade(a=50)", 1000, {
		pax_shade_tri(
			&buf, -1, &shader, NULL,
			0,  0,
			10, 0,
			0,  10
		);
	});
	
	showResults(NULL);
	
	while (1) {
		// Limit of loop.
		bool exuent = false;
		rp2040_read_buttons(&dev_rp2040, &button_bits);
		exuent = !!(button_bits & MASK_BTN_HOME);
		if (exuent) break;
	}
	
}
