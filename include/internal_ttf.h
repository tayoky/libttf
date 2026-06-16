#ifndef _INTERNAL_TTF_H
#define _INTERNAL_TTF_H

#include "ttf.h"

#define FIXED_SCALE 256

typedef int32_t fixed_t;

static inline fixed_t int2fixed(int a) {
	return a * FIXED_SCALE;
}

static inline int fixed2int(fixed_t a) {
	return (a + FIXED_SCALE / 2) / FIXED_SCALE;
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
