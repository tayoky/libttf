#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ttf.h"

struct __attribute__((packed)) bmp_header {
	char magic[2];
	uint32_t size;
	uint32_t reserved;
	uint32_t offset;
};

struct __attribute__((packed)) bmp_info_header {
	uint32_t size;
	uint32_t width;
	uint32_t height;
	uint16_t planes;
	uint16_t bpp;
	uint32_t compression;
	uint32_t image_size;
	uint32_t xpm;
	uint32_t ypm;
	uint32_t colors;
	uint32_t imp;
};

void bmp_save(FILE *file,uint8_t *bitmap,size_t width,size_t height){
	struct bmp_header header;
	header.magic[0] = 'B';
	header.magic[1] = 'M';
	header.size = 14;
	header.reserved = 0;
	header.offset = sizeof(struct bmp_header) + sizeof(struct bmp_info_header) + 256 * 4;
	fwrite(&header,sizeof(header),1,file);
	struct bmp_info_header info_header;
	info_header.size = sizeof(info_header);
	info_header.width = width;
	info_header.height = height;
	info_header.planes = 1;
	info_header.bpp = 8;
	info_header.compression = 0;
	info_header.image_size = width * height;
	info_header.colors = 256;
	info_header.imp = 0;
	fwrite(&info_header,sizeof(info_header),1,file);
	
	//write colors palette
	for(int i=0; i<256; i++){
		uint8_t c=(uint8_t)i;
		fwrite(&c,1,1,file);
		fwrite(&c,1,1,file);
		fwrite(&c,1,1,file);
		c = 0;
		fwrite(&c,1,1,file);
	}

	for(size_t i=0; i<height; i++){
		fwrite(bitmap,width,1,file);
		if(width % 4){
			char buf[4];
			fwrite(buf,4-(width%4),1,file);
		}
		bitmap += width;
	}
}

int main(int argc,char **argv){
	if(argc != 2){
		printf("usage : test FONT\n");
		return 1;
	}

	ttf_file *font = ttf_open(argv[1]);
	if(!font){
		printf("libttf : %s\n",ttf_error());
		return 1;
	}

	printf("a and b are mapped to %d and %d\n",ttf_char2glyph(font,'a'),ttf_char2glyph(font,'b'));

	printf("glyph a stat : \n");
	ttf_glyph *a = ttf_getglyph(font,'A');
	printf("number of contours : %d\n",a->num_contours);
	printf("number of points : %u\n",a->num_pts);
	printf("xmin : %d\n",a->x_min);
	printf("ymin : %d\n",a->y_min);
	printf("xmax : %d\n",a->x_max);
	printf("ymax : %d\n",a->y_max);
	printf("points :\n");
	for(int i = 0; i<a->num_pts; i++){
		printf("x %d y %d\n",a->pts[i].x,a->pts[i].y);
	}


	char path[256];
	sprintf(path,"%s/storage/downloads/out.bmp",getenv("HOME"));
	FILE *out = fopen(path,"w");
	ttf_set_font_size(font,256);
	ttf_bitmap *bmp = ttf_render_glyph(a);
	
	bmp_save(out,bmp->bitmap,bmp->width,bmp->height);
	
	ttf_freeglyph(a);
	ttf_close(font);

	return 0;
}
