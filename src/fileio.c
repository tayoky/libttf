#include <stdio.h>
#include <stdint.h>
#include "fileio.h"

uint8_t read_u8(FILE *file){
	uint8_t u8;
	if(fread(&u8,sizeof(u8),1,file) == 0){
		return 0;
	}
	return u8;
}

uint16_t read_u16(FILE *file){
	return read_u8(file) << 8 | read_u8(file);
}

uint32_t read_u32(FILE *file){
	return read_u16(file) << 16 | read_u16(file);
}
