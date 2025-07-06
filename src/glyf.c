#include <stdlib.h>
#include <stdio.h>
#include <string.h>
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

#define X_SHORT_VECTOR 0x02
#define Y_SHORT_VECTOR 0x04
#define REPEAT_FLAG    0x08
#define X_IS_SAME_OR_POSITIVE_X_SHORT_VECTOR 0x10
#define Y_IS_SAME_OR_POSITIVE_Y_SHORT_VECTOR 0x20

ttf_glyph *ttf_getglyph(ttf_file *font,wchar_t c){
	uint32_t glyph_id = ttf_char2glyph(font,c);
	glyph_seek(font,glyph_id);

	ttf_glyph *glyph = malloc(sizeof(ttf_glyph));
	glyph->font = font;
	glyph->num_contours = read_i16(font->file);
	if(glyph->num_contours < 0){
		glyph->num_contours = -1;
	}
	glyph->x_min = read_i16(font->file);
	glyph->y_min = read_i16(font->file);
	glyph->x_max = read_i16(font->file);
	glyph->y_max = read_i16(font->file);

	//TODO : composite glyph support
	if(glyph->num_contours == -1)return glyph;

	glyph->ends_pts = calloc(sizeof(uint16_t),glyph->num_contours);
	for(int i=0; i<glyph->num_contours; i++){
		glyph->ends_pts[i] = read_u16(font->file);
		glyph->num_pts = glyph->ends_pts[i];
	}
	glyph->num_pts++;

	//for the moment just ignore instructions
	uint16_t instruction_lenght = read_u16(font->file);
	for(int i=0; i<instruction_lenght; i++){
		read_u8(font->file);
	}

	glyph->pts = calloc(sizeof(ttf_point),glyph->num_pts);
	for(int i=0; i<glyph->num_pts; i++){
		uint8_t flags = (glyph->pts[i].flags = read_u8(font->file));
		if(flags & REPEAT_FLAG){
			uint8_t count = read_u8(font->file);
			if(count + i >= glyph->num_pts) count = glyph->num_pts - i;
			while(count > 0){
				i++;
				count--;
				glyph->pts[i].flags = flags;
			}
		}
	}

	int32_t last_x = 0;
	int32_t last_y = 0;
	for(int i=0; i<glyph->num_pts; i++){
		uint8_t flags = glyph->pts[i].flags;
		int32_t x;
		if(flags & X_SHORT_VECTOR){
			x = read_u8(font->file);
			if(!(flags & X_IS_SAME_OR_POSITIVE_X_SHORT_VECTOR)){
				x = -x;
			}

			x += last_x;
		} else {
			if(flags & X_IS_SAME_OR_POSITIVE_X_SHORT_VECTOR){
				x = last_x;
			} else {
				x = last_x + read_i16(font->file);
			}
		}

		glyph->pts[i].x = x;
		last_x = x;
	}
	for(int i=0; i<glyph->num_pts; i++){
		uint8_t flags = glyph->pts[i].flags;
		int32_t y;
		if(flags & Y_SHORT_VECTOR){
			y = read_u8(font->file);
			if(!(flags & Y_IS_SAME_OR_POSITIVE_Y_SHORT_VECTOR)){
				y = -y;
			}

			y += last_y;
		} else {
			if(flags & Y_IS_SAME_OR_POSITIVE_Y_SHORT_VECTOR){
				y = last_y;
			} else {
				y = last_y + read_i16(font->file);
			}
		}

		glyph->pts[i].y = y;
		last_y = y;
	}

	

	return glyph;
}

void ttf_freeglyph(ttf_glyph *glyph){
	free(glyph->ends_pts);
	free(glyph->pts);
	free(glyph);
}
