#include <stdio.h>
#include <stdlib.h>
#include "fileio.h"
#include "ttf.h"


ttf_file *ttf_open(const char *path){
	FILE *file = fopen(path,"r");
	if(!file){
		return NULL;
	}

	ttf_file *font = malloc(sizeof(ttf_file));
	font->file = file;

	uint32_t sfnt_version = read_u32(file);
	uint16_t num_table = read_u16(file);
	read_u16(file);
	read_u16(file);
	read_u16(file);
	printf("%x\n",sfnt_version);
	printf("number of table %d\n",num_table);

	for(int i=0; i<num_table; i++){
		uint32_t tag = read_u32(file);
		uint32_t checksum = read_u32(file);
		uint32_t offset = read_u32(file);
		uint32_t lenght = read_u32(file);
		printf("table %x at %x size %d\n",tag,offset,lenght);
	}

	return font;
}
