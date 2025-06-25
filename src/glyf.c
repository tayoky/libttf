#include <stdlib.h>
#include <stdio.h>
#include "fileio.h"
#include "ttf.h"
#include "internal_ttf.h"

static inline size_t offset_size(ttf_file *font){
	return font->flags & TTF_FLAG_LONG_OFF ? sizeof(uint32_t) : sizeof(uint16_t);
}

static void glyph_seek(ttf_file *font,uint32_t glyf){
	seek(font->file,font->loca.offset + offset_size(font) * glyf);
	uint32_t offset;
	if(font->flags & TTF_FLAG_LONG_OFF){
		offset = read_u32(font->file);
	} else {
		offset = read_u16(font->file);
	}

	seek(font->file,font->glyf.offset + offset);
}

int ttf_parse_glyf(ttf_file *font){
	if(!font->maxp.offset){
		__ttf_error = "no maxp table";
		return -1;
	}

	if(!font->loca.offset){
		__ttf_error = "no loca table";
		return -1;
	}

	seek(font->file,font->maxp.offset);
	
	//version must be 1.0 (0x00010000) for truetype outline
	if(read_u32(font->file) != 0x00010000){
		__ttf_error = "invalid maxp table version";
		return -1;
	}

	uint16_t num_glyph = read_u16(font->file);

	printf("%u glyph\n",num_glyph);

	return 0;
}

ttf_glyph *ttf_getglyph(ttf_file *font,wchar_t c){
	uint32_t glyph_id = ttf_char2glyph(font,c);
	glyph_seek(font,glyph_id);

	ttf_glyph *glyph = malloc(sizeof(ttf_glyph));
	glyph->num_contours = read_i16(font->file);
	glyph->x_min = read_i16(font->file);
	glyph->y_min = read_i16(font->file);
	glyph->x_max = read_i16(font->file);
	glyph->y_max = read_i16(font->file);

	return glyph;
}
