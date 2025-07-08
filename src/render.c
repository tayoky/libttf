#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "ttf.h"
#include "internal_ttf.h"

#define convert(funit) (((funit) * glyph->font->font_size + glyph->font->unit_per_em / 2) / glyph->font->unit_per_em)
#define pix(x,y) (((y) * bmp->width) + (x))

static void check_line(ttf_glyph *glyph,int y,ttf_point *a,ttf_point *b,int *intersections,int *count){
	if(b->y > a->y){
		ttf_point *c = b;
		b = a;
		a = c;
	}
 
	int ax = convert(a->x - glyph->x_min) * 256;
	int ay = convert(a->y - glyph->y_min);
	int bx = convert(b->x - glyph->x_min) * 256;
	int by = convert(b->y - glyph->y_min);
	//we need one below and on top
	if(by + 1 > y){
		return;
	}
	if(ay  < y){
		return;
	}

	//there is an intersection but where ?
	if(ay == by){
		intersections[*count] = ax;
		intersections[(*count)+1] = bx;
		*count += 2;
		return;
	}

	intersections[*count] = bx + (ax - bx) * (y - by) /(ay - by);
	*count += 1;
}

static int check_intersections(ttf_glyph *glyph,int y,int *intersections){
	int count = 0;
	int first = 0;
	for(int i=0; i<glyph->num_contours; i++){
		int last = glyph->ends_pts[i];
		for(int j=first; j<last; j++){
			check_line(glyph,y,&glyph->pts[j],&glyph->pts[j+1],intersections,&count);
		}
		check_line(glyph,y,&glyph->pts[last],&glyph->pts[first],intersections,&count);

		first = last + 1;
	}
	return count;
}


static int compare_int(const void *e1,const void *e2){
	const int *i1 = e1;
	const int *i2 = e2;
	return *i1 - * i2;
}

ttf_bitmap *ttf_render_glyph(ttf_glyph *glyph){
	int width = convert(glyph->x_max + 1- glyph->x_min);
	int height = convert(glyph->y_max + 1 - glyph->y_min);

	ttf_bitmap *bmp = malloc(sizeof(ttf_bitmap));
	bmp->width = width;
	bmp->height = height;
	bmp->bitmap = malloc(width * height);
	memset(bmp->bitmap,0,width * height);

	for(int y = 0; y<height; y++){
		int intersections[16];
		int count = check_intersections(glyph,y,intersections);
		//sorting time
		qsort(intersections,count,sizeof(*intersections),compare_int);
		while(count >= 2){
			int x = intersections[count-2];
			if(x % 256){
				uint8_t old = 
				bmp->bitmap[pix(x/256,y)];
				bmp->bitmap[pix(x/256,y)] = 255 - ((x % 256) * (255 - old) / 255);
				x += 256 - (x % 256);
			}

			while(intersections[count-1] - x >= 256){
				bmp->bitmap[pix(x/256,y)] = 255;
				x+= 256;
			}

			if(x < intersections[count-1]){
				bmp->bitmap[pix(x/256,y)] = intersections[count-1] - x;
			}
			count -= 2;
		}
	}
	return bmp;
}

void ttf_set_font_size(ttf_file *font,int size){
	font->font_size = size;
}
