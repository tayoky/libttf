#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "ttf.h"
#include "internal_ttf.h"

#define convert(funit) (((funit) * glyph->font->font_size + glyph->font->unit_per_em - 1) / glyph->font->unit_per_em)
#define pix(x,y) (((y) * bmp->width) + (x))
#define ON_CURVE_POINT 0x01

static void check_line(ttf_glyph *glyph,int y,ttf_point *a,ttf_point *b,int *intersections,int *count){
	if(b->y > a->y){
		ttf_point *c = b;
		b = a;
		a = c;
	}
 
	int ay = convert(a->y - glyph->y_min);
	int by = convert(b->y - glyph->y_min);
	int ax = convert((a->x - glyph->x_min) * 256);
	int bx = convert((b->x - glyph->x_min) * 256);

	ay--;
	by--;
	//we need one below and on top
	if(by >= y){
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

static ttf_point bezier(int res,ttf_point p[3],int t){
	int mt = res - t;
		long p0 = mt*mt;
		long p1 = 2*mt*t;
		long p2 = t*t;
		return (ttf_point){
			.x = (p[0].x * p0 + p[1].x * p1 + p[2].x * p2) / (res * res),
		.y = (p[0].y * p0 + p[1].y * p1 + p[2].y * p2) / (res * res),
	};
}

static void check_curve(ttf_glyph *glyph,int y,ttf_point p[3],int *intersections,int *count){
	for(int i=0; i<glyph->font->curves_seg; i++){
		ttf_point a = bezier(glyph->font->curves_seg,p,i);
		ttf_point b = bezier(glyph->font->curves_seg,p,i+1);
		check_line(glyph,y,&a,&b,intersections,count);
	}
}

#define pt(index) glyph->pts[first + (((index) - first)%(last - first + 1))]

static void snap(ttf_glyph *glyph,ttf_point *p){
				p->x = (convert(p->x - glyph->x_min) * glyph->font->unit_per_em + glyph->font->font_size - 1) / glyph->font->font_size + glyph->x_min;
				p->y = (convert(p->y - glyph->y_min) * glyph->font->unit_per_em + glyph->font->font_size - 1)/ glyph->font->font_size + glyph->y_min;
}

static int check_intersections(ttf_glyph *glyph,int y,int *intersections){
	int count = 0;
	int first = 0;
	for(int i=0; i<glyph->num_contours; i++){
		int last = glyph->ends_pts[i];
		int j=first;
		for(; j<=last; j++){

			ttf_point pts[3];
			for(size_t i=0; i<3; i++){
				pts[i] = pt(i + j);
			}
			//we want the first to be on curve to get a quadric curve or a line
			int block_1 = 0;
			if(!(pts[0].flags & ON_CURVE_POINT)){
				if(pts[1].flags & ON_CURVE_POINT){
					//we handle this case at the end
					continue;
				}
				//it isen't, make it
				pts[0].flags |= ON_CURVE_POINT;
				pts[0].x = (pts[0].x + pts[1].x) / 2;
				pts[0].y = (pts[0].y + pts[1].y) / 2;
			} else {
				//grid finting, we snap the point
				snap(glyph,&pts[0]);
			}

			//now we are sure the first point is on curve
			//but we also need tje the second or third to be on curve
			//so we only have lines and quadric curves
			if(!(pts[1].flags & ON_CURVE_POINT) && !(pts[2].flags & ON_CURVE_POINT)){
				//we need to make the third point on curve
				//take the mid of of second and third
				pts[2].flags |= ON_CURVE_POINT;
				pts[2].x = (pts[1].x + pts[2].x) / 2;
				pts[2].y = (pts[1].y + pts[2].y) / 2;
				block_1 = 1;

			} else {
				//snap the truly on curve point for grid fitting
				if(pts[1].flags & ON_CURVE_POINT){
					snap(glyph,&pts[1]);
				} else {
					snap(glyph,&pts[2]);
				}
			}
			//we already know first point is on curve
			if(pts[1].flags & ON_CURVE_POINT){
				check_line(glyph,y,&pts[0],&pts[1],intersections,&count);
			} else {
				//quadric curve
				check_curve(glyph,y,pts,intersections,&count);
				//since quadrix cube use 3 points skip one more
				if(!block_1)j++;
			}
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
				uint8_t old = bmp->bitmap[pix(x/256,y)];
				bmp->bitmap[pix(x/256,y)] += ((255 - (x % 256)) * (255 - old) / 255);
				x += 256 - (x % 256);
			}

			while(intersections[count-1] - x >= 256){
				bmp->bitmap[pix(x/256,y)] = 255;
				x+= 256;
			}

			if(x < intersections[count-1]){
				uint8_t old = bmp->bitmap[pix(x/256,y)];
				bmp->bitmap[pix(x/256,y)] += ((intersections[count-1] - x) * (255 - old) / 255);
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
