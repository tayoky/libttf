#include <stdio.h>
#include "fileio.h"
#include "ttf.h"
#include "internal_ttf.h"

int ttf_parse_head(ttf_file *font){

	if(font->head.lenght < 46){
		__ttf_error = "too small head table";
		return -1;
	}

	seek(font->file,font->head.offset);

	//check version
	//major = 1, minor = 0
	if(read_u16(font->file) != 1 || read_u16(font->file) != 0){
		__ttf_error = "invalid head table version";
		return -1;
	}

	//ignore font version and checsum adjustement
	read_u32(font->file);
	read_u32(font->file);

	//check magic number
	if(read_u32(font->file) != 0x5F0F3CF5){
		__ttf_error = "invalid magic number in head table";
		return -1;
	}

	font->flags = read_u16(font->file);
	font->unit_per_em = read_u16(font->file);

	//ignore date
	read_u32(font->file);
	read_u32(font->file);
	read_u32(font->file);
	read_u32(font->file);

	int16_t x_min = read_i16(font->file);
	int16_t y_min = read_i16(font->file);
	int16_t x_max = read_i16(font->file);
	int16_t y_max = read_i16(font->file);
	uint16_t mac_style = read_u16(font->file);
	uint16_t lowet_rec = read_u16(font->file);

	return 0;
}
