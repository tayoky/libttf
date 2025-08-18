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


#define ARG_1_AND_2_ARE_WORDS     0x0001
#define ARGS_ARE_XY_VALUES        0x0002
#define ROUND_XY_TO_GRID          0x0004
#define WE_HAVE_A_SCALE           0x0008
#define MORE_COMPONENTS           0x0020
#define WE_HAVE_AN_X_AND_Y_SCALE  0x0040
#define WE_HAVE_A_TWO_BY_TWO      0x0080
#define WE_HAVE_INSTRUCTIONS      0x0100
#define USE_MY_METRICS            0x0200
#define OVERLAP_COMPOUND          0x0400
#define SCALED_COMPONENT_OFFSET   0x0800
#define UNSCALED_COMPONENT_OFFSET 0x1000

//a value to convert a raw F2SOT14 read as int16_t into its true value
#define F2DOT14 (1 << 14)

ttf_glyph *_ttf_getglyph(ttf_file *font,uint32_t glyph_id){
	glyph_seek(font,glyph_id);

	ttf_glyph *glyph = malloc(sizeof(ttf_glyph));
	memset(glyph,0,sizeof(ttf_glyph));
	glyph->font = font;
	glyph->num_contours = read_i16(font->file);
	if(glyph->num_contours < 0){
		glyph->num_contours = -1;
	}
	glyph->x_min = read_i16(font->file);
	glyph->y_min = read_i16(font->file);
	glyph->x_max = read_i16(font->file);
	glyph->y_max = read_i16(font->file);

	//composite glyph support
	if(glyph->num_contours == -1){
		glyph->num_contours = 0;
		for(;;){
			uint16_t flags = read_u16(font->file);
			uint16_t glyph_id = read_u16(font->file);
			printf("contain %u\n",glyph_id);
			off_t off = ftell(font->file);
			ttf_glyph *child = _ttf_getglyph(font,glyph_id);
			seek(font->file,off);

			int16_t arg1,arg2;
			if(flags & ARG_1_AND_2_ARE_WORDS){
				arg1 = read_i16(font->file);
				arg2 = read_i16(font->file);
			} else {
				puts("short args");
				if(flags & ARGS_ARE_XY_VALUES){
					arg1 = read_i8(font->file);
					arg2 = read_i8(font->file);
				} else {
					arg1 = read_u8(font->file);
					arg2 = read_u8(font->file);
				}
			}


			if(!(flags & ARGS_ARE_XY_VALUES)){
				printf("aligning points\n");
				flags &= ~SCALED_COMPONENT_OFFSET;
				if(arg1 >= glyph->num_pts)arg1 = glyph->num_pts - 1;
				if(arg2 >= child->num_pts)arg2 = child->num_pts - 1;
				int16_t x = glyph->pts[arg1].x - child->pts[arg1].x;
				int16_t y = glyph->pts[arg1].y - child->pts[arg1].y;
				flags |= ARGS_ARE_XY_VALUES;
				arg1 = (uint16_t)x;
				arg2 = (uint16_t)y;
			}

			printf("offset : %d %d\n",arg1,arg2);


			int16_t mat[2][2];
			memset(mat,0,sizeof(mat));
			if(flags & WE_HAVE_A_SCALE){
				mat[0][0] = mat[1][1] = read_u16(font->file);
#ifdef DEBUG
				printf("scale\n");
#endif
			} else if(flags & WE_HAVE_AN_X_AND_Y_SCALE){
				mat[0][0] = read_u16(font->file);
				mat[1][1] = read_u16(font->file);
#ifdef DEBUG
				printf("X and Y scale\n");
#endif
			} else if(flags & WE_HAVE_A_TWO_BY_TWO){
				mat[0][0] = read_u16(font->file);
				mat[1][0] = read_u16(font->file);
				mat[0][1] = read_u16(font->file);
				mat[1][1] = read_u16(font->file);
#ifdef DEBUG
				printf("two by two scale\n");
#endif
			} else {
				mat[0][0] = mat[1][1] = F2DOT14;
			}

			//apply modifications to the childs's point
			for(int i=0; i<child->num_pts; i++){
				int32_t x = child->pts[i].x;
				int32_t y = child->pts[i].y;

				if((flags & SCALED_COMPONENT_OFFSET) && !(flags & UNSCALED_COMPONENT_OFFSET)){
					x += arg1;

					y += arg2;
				}

				x = x * mat[0][0] / F2DOT14 + y * mat[0][1] / F2DOT14;
				y = x * mat[1][0] / F2DOT14 + y * mat[1][1] / F2DOT14;

				if(!(flags & SCALED_COMPONENT_OFFSET) || (flags & UNSCALED_COMPONENT_OFFSET)){
					x += arg1;
					y += arg2;
				}

				child->pts[i].x = x;
				child->pts[i].y = y;
			}

			glyph->pts = realloc(glyph->pts,(glyph->num_pts + child->num_pts) * sizeof(ttf_point));
			memcpy(&glyph->pts[glyph->num_pts],child->pts,child->num_pts * sizeof(ttf_point));
			glyph->num_pts += child->num_pts;
			glyph->ends_pts = realloc(glyph->ends_pts,(glyph->num_contours + child->num_contours) * sizeof(uint16_t));
			memcpy(&glyph->ends_pts[glyph->num_contours],child->ends_pts,child->num_contours * sizeof(uint16_t));
			glyph->num_contours += child->num_contours;
		

			ttf_free_glyph(child);
			if(!(flags & MORE_COMPONENTS)){
				//no more ? quit
				break;
			}
		}
		return glyph;
	}

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

ttf_glyph *ttf_getglyph(ttf_file *font,wchar_t c){
	return _ttf_getglyph(font,ttf_char2glyph(font,c));
}

void ttf_free_glyph(ttf_glyph *glyph){
	free(glyph->ends_pts);
	free(glyph->pts);
	free(glyph);
}
