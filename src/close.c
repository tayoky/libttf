#include <stdio.h>
#include "ttf.h"

void ttf_close(ttf_file *font){
	fclose(font->file);
	free(font->char_mapping);
	free(font);
}
