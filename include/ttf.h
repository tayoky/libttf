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
	uint16_t unit_per_em;
	uint16_t flags;
	uint32_t char_mapping_count;
	ttf_char_mapping *char_mapping;
} ttf_file;

extern const char *__ttf_error;

ttf_file *ttf_open(const char *path);
void ttf_close(ttf_file *file);
const char *ttf_error(void);
uint32_t ttf_char2glyph(ttf_file *font,uint32_t c);

#define TAG(str) (str[0] << 24 | str[1] << 16 | str[2] << 8 | str[3])
#endif
