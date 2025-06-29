#include <stdio.h>
#include <stdlib.h>
#include "ttf.h"

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
	ttf_glyph *a = ttf_getglyph(font,'a');
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
	ttf_freeglyph(a);

	ttf_close(font);

	return 0;
}
