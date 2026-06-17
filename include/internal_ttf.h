#ifndef _INTERNAL_TTF_H
#define _INTERNAL_TTF_H

#include "ttf.h"

#define FIXED_SCALE 256
#define FIXED_ONE   FIXED_SCALE
#define FIXED_MAX   INT32_MAX

typedef int32_t fixed_t;

static inline fixed_t int2fixed(int a) {
	return a * FIXED_SCALE;
}

static inline int fixed2int(fixed_t a) {
	return a / FIXED_SCALE;
}

static inline fixed_t fixed_floor(fixed_t a) {
	return a - (a % FIXED_SCALE);
}

static inline fixed_t fixed_ceil(fixed_t a) {
	if (a % FIXED_SCALE) {
		a += FIXED_SCALE - (a % FIXED_SCALE);
	}
	return a;
}

static inline fixed_t fixed_decimal(fixed_t a) {
	return a % FIXED_SCALE;
}

static inline fixed_t fixed_mul(fixed_t a, fixed_t b) {
	return a * b / FIXED_SCALE;
}

static inline fixed_t fixed_div(fixed_t a, fixed_t b) {
	return a * FIXED_SCALE / b;
}

int ttf_parse_cmap(ttf_file *font);
int ttf_parse_head(ttf_file *font);
int ttf_parse_glyf(ttf_file *font);

#define arraylen(array) (sizeof(array) / sizeof(*array))

#endif
