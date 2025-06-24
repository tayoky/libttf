#include <stdlib.h>
#include "fileio.h"
#include "ttf.h"
#include "internal_ttf.h"
#include <stdio.h>

struct record {
	uint16_t plateform;
	uint16_t encoding;
	uint32_t offset;
};

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
		record.plateform = read_u16(font->file);
		record.encoding = read_u16(font->file);
		record.offset = read_u32(font->file);

		//we want plateform = unicode (0)
		//we also want encoding > 3 (3 and below are deprecated or only for bmp
		if(record.plateform != 0 || record.encoding <= 3)continue;
		best_record = record;
	}

	if(!best_record.offset){
		__ttf_error = "no unicode record in cmap table";
		return -1;
	}

	seek(font->file,font->cmap.offset + best_record.offset);
	uint16_t format = read_u16(font->file);
	read_u16(font->file);
	uint32_t lenght = read_u32(font->file);
	//we ignore the langage
	read_u32(font->file);

	printf("subtable format = %u\n",format);

	switch(format){
	case 12:;
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
		}
		break;
	default:
		__ttf_error = "unknow subtable format in cmap table";
		return -1;
	}
	return 0;
}

uint32_t ttf_char2glyph(ttf_file *font,uint32_t c){
	for(uint32_t i=0; i<font->char_mapping_count; i++){
		if(c >= font->char_mapping[i].start_char && c <= font->char_mapping[i].end_char){
			return c - font->char_mapping[i].start_char + font->char_mapping[i].start_glyph;
		}
	}
	__ttf_error = "no glyph mapped to this char";
	return 0;
}
