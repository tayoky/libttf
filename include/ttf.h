#ifndef _TTF_H
#define _TTF_H

#include <stdio.h>

typedef struct ttf_table_struct {
	uint32_t tag;
	uint32_t checksum;
	uint32_t offset;
	uint32_t lenght;
} ttf_table;

typedef struct ttf_char_mapping_struct {
	uint32_t start_char;
	uint32_t end_char;
	uint32_t start_glyph;
} ttf_char_mapping;

typedef struct ttf_file_struct {
	FILE *file;
	size_t file_size;
	ttf_table head;
	ttf_table cmap;
	ttf_table glyf;
	ttf_table loca;
	ttf_table maxp;
	uint16_t unit_per_em;
	uint16_t reserved;
	uint32_t flags;
	uint32_t char_mapping_count;
	ttf_char_mapping *char_mapping;
} ttf_file;

typedef struct ttf_point_struct {
	int32_t x;
	int32_t y;
	uint8_t flags;
} ttf_point;

typedef struct ttf_glyph_struct {
	int16_t num_contours;
	int16_t x_min;
	int16_t y_min;
	int16_t x_max;
	int16_t y_max;
	uint16_t *ends_pts; //last point of each contour
	uint16_t num_pts;
	ttf_point *pts;
} ttf_glyph;


#define TTF_FLAG_LONG_OFF (1 << 16)

extern const char *__ttf_error;

ttf_file *ttf_open(const char *path);
void ttf_close(ttf_file *file);
const char *ttf_error(void);
uint32_t ttf_char2glyph(ttf_file *font,wchar_t c);
ttf_glyph *ttf_getglyph(ttf_file *font,wchar_t c);
void ttf_freeglyph(ttf_glyph *glyph);

#define TAG(str) (str[0] << 24 | str[1] << 16 | str[2] << 8 | str[3])
#endif
