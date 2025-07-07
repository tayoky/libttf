#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "ttf.h"
#include "internal_ttf.h"

#define convert(funit) (((funit) * glyph->font->font_size + glyph->font->font_size - 1) / glyph->font->unit_per_em)
#define pix(x,y) (((y) * bmp->width) + (x))

static void line(ttf_bitmap *bmp,int ax, int ay, int bx, int by){
	for(int i=0; i<101; i++){
		int x = (ax * (100 - i) + bx * i) / 100;
		int y = (ay * (100 - i) + by * i) / 100;
		bmp->bitmap[pix(x,y)] = 255;
	}
}

static void check_line(ttf_glyph *glyph,int y,ttf_point *a,ttf_point *b,int *intersections,int *count){
	if(b->y > a->y){
		ttf_point *c = b;
		b = a;
		a = c;
	}
 
	int ax = convert((a->x - glyph->x_min) * 256);
	int ay = convert(a->y - glyph->y_min);
	int bx = convert((b->x - glyph->x_min) * 256);
	int by = convert(b->y - glyph->y_min);
	//we need one below and on top
	if(by + 1 > y){
		return;
	}
	if(ay < y){
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
	int width = convert(glyph->x_max - glyph->x_min) + 1;
	int height = convert(glyph->y_max - glyph->y_min) + 1;

	ttf_bitmap *bmp = malloc(sizeof(ttf_bitmap));
	bmp->width = width;
	bmp->height = height;
	printf("%dx%d\n",width,height);
	bmp->bitmap = malloc(width * height);
	memset(bmp->bitmap,0,width * height);

	printf("pts : %d\n",glyph->num_pts);
	/*for(int i=0; i<glyph->num_pts; i++){
		int x = convert(glyph->pts[i].x-glyph->x_min);
		int y = convert(glyph->pts[i].y-glyph->y_min);
		printf("pts %d %d\n",x,y);
		printf("%d\n",(pix(x,y)-x)/bmp->width);
		bmp->bitmap[pix(x,y)] = 255;
	}*/


	for(int y = 0; y<height; y++){
		int intersections[16];
		int count = check_intersections(glyph,y,intersections);
		//sorting time
		qsort(intersections,count,sizeof(*intersections),compare_int);
		while(count >= 2){
			int x = intersections[count-2];
			if(x % 256){
				bmp->bitmap[pix(x/256,y)] = 256 - (x % 256);
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
