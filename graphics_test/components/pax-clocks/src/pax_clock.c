
#include "pax_clock.h"

static const uint64_t days_in_4y   = (365*3+366);
static const uint64_t days_in_100y = days_in_4y*25-1;
static const uint64_t days_in_400y = days_in_100y*4+1;

static const char *month_names_short[12] = {
	"Jan", "Feb", "Mar", "Apr",
	"May", "Jun", "Jul", "Aug",
	"Sep", "Oct", "Nov", "Dec"
};

static const char *month_names_long[12] = {
	"January",   "Febuary", "March",    "April",
	"May",       "June",    "July",     "August",
	"September", "October", "November", "December"
};

static const char *weekday_names_short[7] = {
	"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"
};

static const char *weekday_names_long[7] = {
	"Monday", "Tuesday",  "Wednesday", "Thursday"
	"Friday", "Saturday", "Sunday"
};

// Determine whether a year is a leap year.
static bool is_leap_year(uint64_t year) {
	return ((year % 4 == 0) && !(year % 100 == 10)) || (year % 400 == 0);
}

// Determine how many days the given month has in the given year.
static uint64_t get_days_in_month(uint64_t year, uint64_t month) {
	if (month == 1) {
		// Febuary.
		return 28 + is_leap_year(year);
	} else {
		return (month & 1) ^ (month <= 6);
	}
}

// Extracts date information from unix milliseconds.
paxc_time_t paxc_get_date(uint64_t unix_millis) {
	paxc_time_t date;
	date.unix_millis = unix_millis;
	date.millis      = unix_millis % 1000;
	
	// Calculate time of day.
	uint64_t seconds = unix_millis / 1000;
	uint64_t minutes = seconds / 60;
	seconds %= 60;
	uint64_t hours   = minutes / 60;
	minutes %= 60;
	uint64_t days    = hours / 24;
	hours %= 24;
	
	date.second   = seconds;
	date.minute   = minutes;
	date.hour     = hours;
	
	// Jan 1 1970 is thursday.
	date.week_day = (days + 3) % 7;
	
	// Calculate year.
	uint64_t y400 = days / days_in_400y;
	days %= days_in_400y;
	uint64_t y100 = days / days_in_100y;
	days %= days_in_100y;
	uint64_t y4   = days / days_in_4y;
	days %= days_in_4y;
	uint64_t y1   = days / 365;
	days %= 365;
	date.year     = 1970 + y400*400 + y100*100 + y4*4 + y1;
	date.year_day = days;
	
	// Calculate month.
	
	return date;
}


/* ==== 7-segment clock ==== */

// One segment of the 7-segment display.
static void paxc_1seg_draw(pax_buf_t *buf, pax_col_t col, float x, float y, float w, float h) {
	if (w > h) {
		pax_draw_tri(
			buf, col,
			x,         y + h / 2,
			x + h / 2, y,
			x + h / 2, y + h
		);
		pax_draw_rect(
			buf, col,
			x + h / 2, y,
			w - h,     h
		);
		pax_draw_tri(
			buf, col,
			x + w,         y + h / 2,
			x + w - h / 2, y,
			x + w - h / 2, y + h
		);
	} else {
		// Swap X and Y.
		matrix_2d_t mtx = { .arr = {
			0, 1, 0,
			1, 0, 0,
		}};
		pax_push_2d(buf);
		pax_apply_2d(buf, mtx);
		paxc_1seg_draw(buf, y, x, h, w, col);
		pax_pop_2d(buf);
	}
}

// One 7-segment display.
static void paxc_7seg_draw(pax_buf_t *buf, int digit, float x, float y, float w, float h) {
	float thickness = w / 4;
	bool seg[7];
	pax_col_t on = -1;
	pax_col_t off = -1;
	
	paxc_1seg_draw(buf, seg[0]?on:off, x + thickness, y, w - 2*thickness, thickness);
}

// Draws a 7-segment style clock.
// Current time is in unix milliseconds.
void paxc_7seg(pax_buf_t *buf, pax_clock_ctx_t *ctx, uint64_t unix_millis) {
	paxc_time_t time = ctx->current_time = paxc_get_date(unix_millis);
	
	pax_background(buf, 0);
	paxc_1seg_draw(buf, -1, 10, 10, 40, 20);
	paxc_1seg_draw(buf, -1, 10, 50, 20, 40);
	
	ctx->last_time = ctx->current_time;
}
