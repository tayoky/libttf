#ifndef _TTF_H
#define _TTF_H

#include <stdio.h>


typedef struct ttf_file_struct {
	FILE *file;
} ttf_file;

ttf_file *ttf_open(const char *path);
void ttf_close(ttf_file *file);

#endif
