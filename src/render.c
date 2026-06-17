#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "ttf.h"
#include "internal_ttf.h"

#define convert(funit) ((((long)(funit) * 256) * glyph->font->font_size + glyph->font->unit_per_em / 2) / glyph->font->unit_per_em)
#define pix(x,y) (((y) * bmp->width) + (x))
#define ON_CURVE_POINT 0x01

// based on freetype's algo

typedef struct cell {
	struct cell *next;
	int x;
	fixed_t coverage;
	fixed_t area;
} cell_t;


static cell_t *get_cell(cell_t **grid, int x, int y) {
	//printf("cell %d %d\n", x, y);
	for (cell_t *current = grid[y]; current; current = current->next) {
		if (current->x == x) {
			// cache hit
			return current;
		}
	}
	return NULL;
}

static cell_t *add_cell(cell_t **grid, int x, int y) {
	cell_t *new_cell = get_cell(grid, x, y);
	if (new_cell) return new_cell;
	new_cell = malloc(sizeof(cell_t));
	new_cell->x = x;
	new_cell->next  = grid[y];
	new_cell->coverage = 0; 
	new_cell->area  = 0;
	grid[y] = new_cell;
	return new_cell;
}

static fixed_t calc_area(fixed_t ax, fixed_t ay, fixed_t bx, fixed_t by) {
	fixed_t total_y = by + ay;
	return fixed_mul(bx - ax, total_y) / 2;
}

static void set_cell(cell_t **grid, int cell_x, int cell_y, fixed_t ax, fixed_t ay, fixed_t bx, fixed_t by) {
	cell_t *cell = add_cell(grid, cell_x, cell_y);
	fixed_t fixed_cell_x = int2fixed(cell_x);
	fixed_t fixed_cell_y = int2fixed(cell_y);
	fixed_t local_ax = ax - fixed_cell_x;
	fixed_t local_ay = ay - fixed_cell_y;
	fixed_t local_bx = bx - fixed_cell_x;
	fixed_t local_by = by - fixed_cell_y;
	printf("in cell : %d %d %d %d\n", local_ax, local_ay, local_bx, local_by);
	cell->coverage += local_by - local_ay;
	cell->area += calc_area(local_ax, local_ay, local_bx, local_by);
}

static void check_line(ttf_glyph *glyph, cell_t **grid, ttf_point *a, ttf_point *b){
	fixed_t ax = convert(a->x - glyph->x_min);
	fixed_t ay = convert(a->y - glyph->y_min);
	fixed_t bx = convert(b->x - glyph->x_min);
	fixed_t by = convert(b->y - glyph->y_min);
	fixed_t dx = bx - ax;
	fixed_t dy = by - ay;

	fixed_t dir_x = dx > 0 ? 1 : -1;
	fixed_t dir_y = dy > 0 ? 1 : -1;

	// loop until in target pixel
	fixed_t x = ax;
	fixed_t y = ay;
	int cell_x = fixed2int(x);
	int cell_y = fixed2int(y);
	int target_x = fixed2int(bx);
	int target_y = fixed2int(by);
	// a bit of a special case
	if (dx < 0 && fixed_decimal(x) == 0) cell_x--;
	if (dy < 0 && fixed_decimal(y) == 0) cell_y--;
	printf("line from %d %d to %d %d\n", cell_x, cell_y, target_x, target_y);
	while (cell_x != target_x || cell_y != target_y) {
		fixed_t floor_x = int2fixed(cell_x);
		fixed_t floor_y = int2fixed(cell_y);
		fixed_t next_grid_x = dx > 0 ? floor_x + FIXED_ONE : floor_x;
		fixed_t next_grid_y = dy > 0 ? floor_y + FIXED_ONE : floor_y;

		fixed_t tx = dx != 0 ? fixed_div(next_grid_x - x, dx) : FIXED_MAX;
		fixed_t ty = dy != 0 ? fixed_div(next_grid_y - y, dy) : FIXED_MAX;
		fixed_t next_x, next_y;
		int next_cell_x = cell_x;
		int next_cell_y = cell_y;
		if (tx == ty) {
			next_x = next_grid_x;
			next_y = next_grid_y;
			next_cell_x += dir_x;
			next_cell_y += dir_y;
		} else if (tx < ty) {
			// vertical line hit
			next_x = next_grid_x;
			next_y = y + fixed_mul(tx, dy);
			next_cell_x += dir_x;
		} else {
			// horizontal line hit
			next_x = x + fixed_mul(ty, dx);
			next_y = next_grid_y;
			next_cell_y += dir_y;
		}
		printf("at %d %d next is %d %d\n", fixed2int(x), fixed2int(y), fixed2int(next_x), fixed2int(next_y));
		set_cell(grid, cell_x, cell_y, x, y, next_x, next_y);
		cell_x = next_cell_x;
		cell_y = next_cell_y;
		x = next_x;
		y = next_y;
	}
	set_cell(grid, cell_x, cell_y, x, y, bx, by);
	return;
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

static void check_curve(ttf_glyph *glyph, cell_t **grid, ttf_point p[3]) {
	for(int i=0; i<glyph->font->curves_seg; i++){
		ttf_point a = bezier(glyph->font->curves_seg,p,i);
		ttf_point b = bezier(glyph->font->curves_seg,p,i+1);
		check_line(glyph, grid, &a, &b);
	}
}

#define pt(index) glyph->pts[first + (((index) - first)%(last - first + 1))]

static int check_intersections(ttf_glyph *glyph, cell_t **grid) {
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

			}
			//we already know first point is on curve
			if(pts[1].flags & ON_CURVE_POINT){
				check_line(glyph, grid, &pts[0], &pts[1]);
			} else {
				//quadric curve
				check_curve(glyph, grid, pts);
				//since quadrix cube use 3 points skip one more
				if(!block_1)j++;
			}
		}

		first = last + 1;
	}
	return count;
}

ttf_bitmap *ttf_render_glyph(ttf_glyph *glyph){
	int width = fixed2int(fixed_ceil(convert(glyph->x_max - glyph->x_min))) + 1;
	int height = fixed2int(fixed_ceil(convert(glyph->y_max - glyph->y_min))) + 1;

	ttf_bitmap *bmp = malloc(sizeof(ttf_bitmap));
	bmp->width = width;
	bmp->height = height;
	bmp->bitmap = malloc(width * height);
	memset(bmp->bitmap,0,width * height);

	cell_t *grid[height];
	memset(grid, 0, sizeof(grid));
	check_intersections(glyph, grid);

	for(int y = 0; y<height; y++){
		fixed_t coverage = 0;
		for (int x=0; x<width; x++) {
			cell_t *cell = get_cell(grid, x, y);
			fixed_t area = 0;
			if (cell) {
				coverage += cell->coverage;
				area = cell->area;
			}
			fixed_t local_coverage = coverage - area;
			if (local_coverage <= 0) {
				bmp->bitmap[pix(x, y)] = 0;
			} else if (local_coverage < FIXED_ONE) {
				bmp->bitmap[pix(x, y)] = local_coverage;
			} else {
				bmp->bitmap[pix(x, y)] = 255;
			}
		}
	}
	// TODO : free grid
	return bmp;
}

void ttf_set_font_size(ttf_file *font,int size){
	font->font_size = size;
}

void ttf_set_curves_seg(ttf_file *font,int count){
	font->curves_seg = count;
}
