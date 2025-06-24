#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fileio.h"
#include "ttf.h"
#include "internal_ttf.h"

uint32_t calc_checksum(ttf_file *file,ttf_table *table){
	seek(file->file,table->offset);
	uint32_t lenght = table->lenght;
	uint32_t checksum = 0;
	while(lenght > sizeof(uint32_t)){
		checksum += read_u32(file->file);
		lenght -= sizeof(uint32_t);
	}

	if(lenght > 0){
		uint32_t last = 0;
		int shift = 24;
		while(lenght > 0){
			last |= read_u8(file->file) << shift;
			shift -= 8;
			lenght--;
		}
		checksum += last;
	}

	//checksum adjustement for table head
	if(table->tag == TAG("head")){
		seek(file->file,table->offset + 8);
		checksum -= read_u32(file->file);
	}
	return checksum;
}

ttf_file *ttf_open(const char *path){
	FILE *file = fopen(path,"r");
	if(!file){
		__ttf_error = "no such file";
		return NULL;
	}

	ttf_file *font = malloc(sizeof(ttf_file));
	memset(font,0,sizeof(ttf_file));
	font->file = file;

	uint32_t sfnt_version = read_u32(file);
	uint16_t num_table = read_u16(file);
	read_u16(file);
	read_u16(file);
	read_u16(file);
	printf("%x\n",sfnt_version);
	printf("number of table %d\n",num_table);

	for(int i=0; i<num_table; i++){
		ttf_table table;
		memset(&table,0,sizeof(table));
		seek(file,12 + i * 16);
		table.tag = read_u32(file);
		table.checksum = read_u32(file);
		table.offset = read_u32(file);
		table.lenght = read_u32(file);
		printf("table %x at %x size %d\n",table.tag,table.offset,table.lenght);
		if(calc_checksum(font,&table) != table.checksum){
			__ttf_error = "invalid checksum";
			goto error;
		}

		switch(table.tag){
		case TAG("head"):
			font->head = table;
			break;
		case TAG("cmap"):
			font->cmap = table;
			break;
		case TAG("glyf"):
			font->glyf = table;
			break;
		}
	}

	if(!font->head.offset){
		__ttf_error = "no head table";
		goto error;
	}
	if(!font->cmap.offset){
		__ttf_error = "no cmap table";
		goto error;
	}

	//now we can start to parse the tables
	if(font->head.lenght < 46){
		__ttf_error = "too small head table";
		goto error;
	}
	seek(file,font->head.offset);
	//major = 1, minor = 0
	if(read_u16(file) != 1 || read_u16(file) != 0){
		__ttf_error = "invalid head table version";
		goto error;
	}
	//ignore font version and checsum adjustement
	read_u32(file);
	read_u32(file);
	//check magic number
	if(read_u32(file) != 0x5F0F3CF5){
		__ttf_error = "invalid magic number in head table";
		goto error;
	}
	uint16_t flags = read_u16(file);
	uint16_t unit_per_em = read_u16(file);
	//ignore date
	read_u32(file);
	read_u32(file);
	read_u32(file);
	read_u32(file);

	int16_t x_min = read_i16(file);
	int16_t y_min = read_i16(file);
	int16_t x_max = read_i16(file);
	int16_t y_max = read_i16(file);
	uint16_t mac_style = read_u16(file);
	uint16_t lowet_rec = read_u16(file);

	if(ttf_parse_cmap(font) < 0){
		goto error;
	}

	return font;
error:
	if(font->char_mapping)free(font->char_mapping);
	fclose(file);
	free(font);
	return NULL;
}
