
#ifndef PAX_CLOCK_H
#define PAX_CLOCK_H

#include <pax_gfx.h>
#include <stdint.h>

typedef struct {
	/* ==== Unix timestamp ==== */
	// Unix milliseconds (millis since Jan 01 1970 00:00:00).
	uint64_t unix_millis;
	
	/* ==== Date ==== */
	// Years since 0000 ;)
	uint16_t year;
	// Months zero-indexed.
	uint8_t  month;
	// Day of year zero-indexed.
	uint16_t year_day;
	// Day of month zero-indexed.
	uint8_t  month_day;
	// Day of week zero-indexed.
	uint8_t  week_day;
	
	/* ==== Time of day ==== */
	// Hours ranging 0-23.
	uint8_t  hour;
	// Minutes ranging 0-59.
	uint8_t  minute;
	// Seconds ranging 0-59.
	uint8_t  second;
	// Milliseconds ranging 0-999.
	uint16_t millis;
} paxc_time_t;

typedef struct {
	/* ==== Timekeeping ==== */
	paxc_time_t last_time;
	paxc_time_t current_time;
	
} pax_clock_ctx_t;

// Extracts date information from unix milliseconds.
paxc_time_t paxc_get_date(uint64_t unix_millis);

// Draws a 7-segment style clock.
// Current time is in unix milliseconds.
void paxc_7seg(pax_buf_t *buf, pax_clock_ctx_t *ctx, uint64_t unix_millis);

#endif //PAX_CLOCK_H
