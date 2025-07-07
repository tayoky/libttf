#ifndef _INTERNAL_TTF_H
#define _INTERNAL_TTF_H

#include "ttf.h"

int ttf_parse_cmap(ttf_file *font);
int ttf_parse_head(ttf_file *font);
int ttf_parse_glyf(ttf_file *font);

#define arraylen(array) (sizeof(array) / sizeof(*array))

#endif
