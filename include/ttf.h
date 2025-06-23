#ifndef _TTF_H
#define _TTF_H

#include <stdio.h>

typedef struct ttf_table_struct {
	uint32_t tag;
	uint32_t checksum;
	uint32_t offset;
	uint32_t lenght;
} ttf_table;

typedef struct ttf_file_struct {
	FILE *file;
} ttf_file;

ttf_file *ttf_open(const char *path);
void ttf_close(ttf_file *file);

#define TAG(str) (str[0] << 24 | str[1] << 16 | str[2] << 8 | str[3])
#endif
