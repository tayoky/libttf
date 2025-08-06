#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "ttf.h"
#include "internal_ttf.h"

#define convert(funit) (((funit) * glyph->font->font_size + glyph->font->unit_per_em / 2) / glyph->font->unit_per_em)
#define pix(x,y) (((y) * bmp->width) + (x))
#define ON_CURVE_POINT 0x01

static void check_line(ttf_glyph *glyph,int y,ttf_point *a,ttf_point *b,int *intersections,int *count,int snap){
	if(b->y > a->y){
		ttf_point *c = b;
		b = a;
		a = c;
	}
 
	int ay = convert(a->y - glyph->y_min);
	int by = convert(b->y - glyph->y_min);
	int ax,bx; 
	if(snap){
		ax = convert(a->x - glyph->x_min) * 256;
		bx = convert(b->x - glyph->x_min) * 256;
	} else {

		ax = convert((a->x - glyph->x_min) * 256);
		bx = convert((b->x - glyph->x_min) * 256);
	}
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

static inline long fact(long x){
	long ret = 1;
	for(long i=2; i<=x; i++){
		ret *= i;
	}
	return ret;
}
static inline long C(long x,long y){
	return fact(x) / (fact(y) * fact(x - y));
}
static ttf_point bezier(int res,ttf_point **p,size_t pts_num,int t){
	ttf_point ret = {.x=0,.y=0};
	for(size_t i=0; i<pts_num; i++){
		long coef = C(pts_num-1,i);
		for(size_t j=0; j<pts_num-1; j++){
			if(j < i){
				coef *= t;
			} else {
				coef *= res - t;
			}
		}
		ret.x += p[i]->x * coef;
		ret.y += p[i]->y * coef;
	}
	for(size_t i=0; i<pts_num-1; i++){
		ret.x /= res;
		ret.y /= res;
	}
	return ret;
}

static void check_curve(ttf_glyph *glyph,int y,ttf_point **p,size_t pts_num,int *intersections,int *count){
	//uh idk what i am doing
	//long A = -p[0].y + 3*p[1].y - 3*p[2].y + p[3].y;
	//long B = 3*p[0].y - 6*p[1].y + 2*p[2].y;
	//long C = -3*p[0].y + 3*p[1].y;
	//long D = p[0].y - y;

	//now At ^ 3 + Bt ^ 2 + Ct + D = 0
	//we need to find t
	for(int i=0; i<glyph->font->curves_seg; i++){
		ttf_point a = bezier(glyph->font->curves_seg,p,pts_num,i);
		ttf_point b = bezier(glyph->font->curves_seg,p,pts_num,i+1);
		check_line(glyph,y,&a,&b,intersections,count,0);
	}
}

static int check_intersections(ttf_glyph *glyph,int y,int *intersections){
	int count = 0;
	int first = 0;
	for(int i=0; i<glyph->num_contours; i++){
		int last = glyph->ends_pts[i];
		int j=first;
		while(j < last && !(glyph->pts[j].flags & ON_CURVE_POINT))j++;
		for(; j<=last; j++){
			//find the number of point on current curve
			size_t pts_num = 1;
			while(!(glyph->pts[first + ((j + pts_num - first)%(last - first + 1))].flags & ON_CURVE_POINT)){
				pts_num++;
			}
			pts_num++;

			ttf_point *pts[pts_num];
			for(size_t i=0;i<pts_num;i++){
				pts[i] = &glyph->pts[first + ((j + i - first)%(last - first + 1))];
			}

			//printf("curve %zu points\n",pts_num);
			if(pts_num == 2){
				check_line(glyph,y,pts[0],pts[1],intersections,&count,1);
			} else {
				check_curve(glyph,y,pts,pts_num,intersections,&count);
			}

			j+= pts_num  - 2;
		}

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

void ttf_set_curves_seg(ttf_file *font,int count){
	font->curves_seg = count;
}
