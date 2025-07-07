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

	read_i16(font->file);
	read_i16(font->file);
	read_i16(font->file);
	read_i16(font->file);
	read_u16(font->file); //mac style
	read_u16(font->file); //lowest readable resolution
	read_u16(font->file); //font direction hint
	//index to loc format
	switch(read_u16(font->file)){
	case 0:
		break;
	case 1:
		font->flags |= TTF_FLAG_LONG_OFF;
		break;
	default:
		__ttf_error = "invalid indexToLocFormat in head table";
		return -1;
	}

	return 0;
}
