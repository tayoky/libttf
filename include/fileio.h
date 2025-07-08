#ifndef _FILEIO_H
#define _FILEIO_H

#include <stdint.h>
#include <stdio.h>

uint8_t read_u8(FILE*);
uint16_t read_u16(FILE*);
uint32_t read_u32(FILE*);
void seek(FILE *file,uint32_t offset);

#define read_i8 (int16_t)read_u8
#define read_i16 (int16_t)read_u16

#endif
