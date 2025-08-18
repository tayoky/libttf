#include <stdlib.h>
#include "fileio.h"
#include "ttf.h"
#include "internal_ttf.h"
#include <stdio.h>

struct record {
	uint16_t plateform;
	uint16_t encoding;
	uint32_t offset;
	uint16_t format;
};

//if a format good ? and how much
static int good(uint16_t format){
	switch(format){
	case 12:
		return 13;
	case 13:
		return 12;
	case 4:
		return 4;
	default:
		return -1;
	}
}

int ttf_parse_cmap(ttf_file *font){
	if(font->cmap.lenght < 4){
		__ttf_error = "too small cmap table";
		return -1;
	}

	seek(font->file,font->cmap.offset);
	//version must be 0x0000
	if(read_u16(font->file) != 0x0000){
		__ttf_error = "invalid cmap table version";
		return -1;
	}

	uint16_t num_record = read_u16(font->file);
	if(font->cmap.lenght < 4 + num_record * 8){
		__ttf_error = "too small cmap table";
		return -1;
	}

	printf("find %d encoding\n",num_record);
	struct record best_record = {.offset = 0};
	for(int i=0; i<num_record; i++){
		struct record record;
		seek(font->file,font->cmap.offset + 4 + i * 8);
		record.plateform = read_u16(font->file);
		record.encoding = read_u16(font->file);
		record.offset = read_u32(font->file);
		seek(font->file,font->cmap.offset + record.offset);
		record.format = read_u16(font->file);


		//if we don't support skip
		if(good(record.format) == -1){
#ifdef DEBUG
			printf("found unsupported char mapping format %u\n",record.format);
#endif
			continue;
		}

		if(best_record.offset){
			//is this better than what we already have
			if(good(record.format) < good(best_record.format)){
				continue;
			}
		}
		best_record = record;
	}

	if(!best_record.offset){
		__ttf_error = "unsupported records in cmap table";
		return -1;
	}

	seek(font->file,font->cmap.offset + best_record.offset);
	uint16_t format = read_u16(font->file);
	read_u16(font->file);
	uint32_t lenght = read_u32(font->file);
	//we ignore the langage
	read_u32(font->file);

	switch(format){
	case 12:
	case 13:;
		uint32_t num_group = read_u32(font->file);
		if(lenght < 16 + 12 * num_group){
			__ttf_error = "too small subtable in cmap table";
			return -1;
		}
		font->char_mapping_count = num_group;
		font->char_mapping = calloc(sizeof(ttf_char_mapping),num_group);
		for(uint32_t i=0; i<num_group; i++){
			font->char_mapping[i].start_char = read_u32(font->file);
			font->char_mapping[i].end_char = read_u32(font->file);
			font->char_mapping[i].start_glyph = read_u32(font->file);
			font->char_mapping[i].type = format == 12 ? TTF_CMAP_INC : TTF_CMAP_SAME;
		}
		break;
	case 4:;
		uint16_t seg_count = read_u16(font->file) / 2;
		read_u16(font->file);
		read_u16(font->file);
		read_u16(font->file);
		font->char_mapping_count = seg_count;
		font->char_mapping = calloc(sizeof(ttf_char_mapping),seg_count);
		for(int i=0; i<seg_count; i++){
			font->char_mapping[i].type = TTF_CMAP_INC;
			font->char_mapping[i].flags = TTF_CMAP_MOD16;
			font->char_mapping[i].end_char = read_u16(font->file);
		}
		read_u16(font->file);
		for(int i=0; i<seg_count; i++){
			font->char_mapping[i].start_char = read_u16(font->file);
		}
		for(int i=0; i<seg_count; i++){
			font->char_mapping[i].start_glyph = font->char_mapping[i].start_char + read_i16(font->file);
		}
		return 0;
		if(read_u16(font->file) != 0){
			__ttf_error = "unsupported subtable of format 4 in cmap table";
			return -1;
		}

		break;
	}
	return 0;
}

uint32_t ttf_char2glyph(ttf_file *font,uint32_t c){
	for(uint32_t i=0; i<font->char_mapping_count; i++){
		switch(font->char_mapping[i].type){
		case TTF_CMAP_INC:
			if(c >= font->char_mapping[i].start_char && c <= font->char_mapping[i].end_char){
				uint32_t id = c - font->char_mapping[i].start_char + font->char_mapping[i].start_glyph;
				return font->char_mapping[i].flags & TTF_CMAP_MOD16 ? id % 65536 : id;
			}
			break;
		case TTF_CMAP_SAME:
			if(c >= font->char_mapping[i].start_char && c <= font->char_mapping[i].end_char){
				return font->char_mapping[i].start_glyph;
			}
			break;
		}
	}
	__ttf_error = "no glyph mapped to this char";
	return 0;
}
