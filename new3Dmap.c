#include <stdlib.h>

#define SCREEN_W   320
#define SCREEN_H   240
#define BLACK 0x0000
#define TILE 20
#define COLS 10
#define ROWS 10
#define NUM_MAPS 6
#define BALL_SIZE 8
#define SPEED 8

#define MAP_OFFSET_X  ((SCREEN_W - COLS*TILE) / 2)
#define MAP_OFFSET_Y  ((SCREEN_H - ROWS*TILE) / 2)

// colors 
#define COL_BG 0x0000  // pure black
#define COL_WALL_OUTER 0x4208  // dark gray border
#define COL_WALL_CORE 0xFFFF  // white center
#define COL_WALL_SHINE 0xAD55  // light blue-gray highlight
#define COL_BALL_OUT 0xF800  // red outer ring
#define COL_BALL_MID 0xFC10  // orange mid
#define COL_BALL_IN 0xFFF0  // bright yellow core
#define COL_TGT_OUT 0x07FF  // cyan outer
#define COL_TGT_MID 0x07E0  // green mid
#define COL_TGT_IN 0xFFFF  // white center dot
#define COL_TGT_PULSE 0x03EF  // teal pulse ring

#define WHITE 0xFFFF
#define RED 0xF800
#define GREEN 0x07E0
#define BLUE 0x001F
#define YELLOW 0xFFE0 
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define ORANGE 0xFD20
#define PINK 0xF81F
#define PURPLE 0x8010

short int Buffer1[240][512];
int target_col, target_row;

int maps[NUM_MAPS][ROWS][COLS] = {
{   // MAP 0: box with inner cross
    {1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,0,0,1},
    {1,0,1,1,0,0,1,1,0,1},
    {1,0,1,0,0,0,0,1,0,1},
    {1,0,0,0,1,1,0,0,0,1},
    {1,0,0,0,1,1,0,0,0,1},
    {1,0,1,0,0,0,0,1,0,1},
    {1,0,1,1,0,0,1,1,0,1},
    {1,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1},
},{  // MAP 1: zigzag corridors
    {1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,1,0,0,0,1},
    {1,0,1,1,0,1,0,1,0,1},
    {1,0,1,0,0,0,0,1,0,1},
    {1,0,1,0,1,1,1,1,0,1},
    {1,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,0,1,0,1,1,1},
    {1,0,0,0,0,1,0,0,0,1},
    {1,0,1,1,1,1,0,1,0,1},
    {1,1,1,1,1,1,1,1,1,1},
},{  // MAP 2: ring with holes
    {1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,0,0,1},
    {1,0,1,1,1,1,1,1,0,1},
    {1,0,1,0,0,0,0,0,0,1},
    {1,0,1,0,1,1,0,1,0,1},
    {1,0,1,0,1,1,0,1,0,1},
    {1,0,0,0,0,0,0,1,0,1},
    {1,0,1,1,1,1,1,1,0,1},
    {1,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1},
},{  // MAP 3: chunks
    {1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,1,0,0,0,0,1},
    {1,0,1,0,1,0,1,1,0,1},
    {1,0,1,0,0,0,0,1,0,1},
    {1,1,1,0,1,0,0,1,0,1},
    {1,0,0,0,1,0,1,1,0,1},
    {1,0,1,1,1,0,0,0,0,1},
    {1,0,0,0,0,0,1,1,0,1},
    {1,0,1,1,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1},
},{  // MAP 4: randomm
    {1,1,1,1,1,1,1,1,1,1},
    {1,0,0,1,0,0,1,0,0,1},
    {1,0,0,0,0,0,1,0,0,1},
    {1,0,0,1,0,0,0,0,0,1},
    {1,1,0,1,1,1,1,0,1,1},
    {1,1,0,1,1,1,1,0,1,1},
    {1,0,0,0,0,0,1,0,0,1},
    {1,0,0,1,0,0,0,0,0,1},
    {1,0,0,1,0,0,1,0,0,1},
    {1,1,1,1,1,1,1,1,1,1},
},{  // MAP 5: pillars
    {1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,0,0,1},
    {1,0,1,0,0,1,0,0,1,1},
    {1,0,0,1,0,0,0,0,0,1},
    {1,0,0,1,0,0,0,1,0,1},
    {1,0,0,0,1,1,0,0,0,1},
    {1,0,1,0,0,0,0,0,0,1},
    {1,1,0,0,1,0,0,1,0,1},
    {1,0,0,0,1,0,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1},
},};

void plot_pixel(int x, int y, short int color) {
	if(x<0) {return;}
	if(x>319) {return;}
	if(y<0) {return;}
	if (y>239) {return;}
	
    if (x < 0 || x >= SCREEN_W || y < 0 || y >= SCREEN_H) return;
    Buffer1[y][x] = color;
}

void draw_rect(int x, int y, int w, int h, short int color) {
    for (int r = 0; r < h; r++)
        for (int c = 0; c < w; c++)
            plot_pixel(x+c, y+r, color);
}

//3d graphing code********************************************
void line(int x1, int y1, int x2, int y2, short color);
void quad(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, short color);

#define CAMERA_DISTANCE 135
#define HALF_BOX_WIDTH 16

struct TwoDPoint {
	int x;
	int y;                                                                                                                                                                                   
};

struct BoxPoints{
	
	struct TwoDPoint ftl;
	struct TwoDPoint ftr;
	struct TwoDPoint fbl;
	struct TwoDPoint fbr;
	struct TwoDPoint btl;
	struct TwoDPoint btr;
	struct TwoDPoint bbl;
	struct TwoDPoint bbr;

};

struct TwoDPoint projectPoint(int x, int y, int z);

struct BoxPoints getBoxPoints(int x, int y, int z);

void drawBox(int x, int y, int z, short color);

int abs(int x);
void swap(int *x, int *y);
//******************************************************************

// dark outer border → solid white core → 1px shine top-left
void draw_wall_tile(int col, int row) {
    int x =  col * 3*TILE-260;
    int y =  row * 3*TILE-270;

    drawBox(x,y,-250,0x9534);
}

// circle
void draw_circle(int cx, int cy, int r, short int color) {
    int x = 0, y = r, d = 1 - r;
    while (x <= y) {
        plot_pixel(cx+x,cy+y,color); 
		plot_pixel(cx-x,cy+y,color);
        plot_pixel(cx+x,cy-y,color); 
		plot_pixel(cx-x,cy-y,color);
        plot_pixel(cx+y,cy+x,color); 
		plot_pixel(cx-y,cy+x,color);
        plot_pixel(cx+y,cy-x,color); 
		plot_pixel(cx-y,cy-x,color);
        
		if (d < 0) 
			d += 2*x+3; 
		else { 
			d += 2*(x-y)+5; y--; 
		} 
		x++;
    }
}

// filled circle
void fill_circle(int cx, int cy, int r, short int color) {
    for (int dy = -r; dy <= r; dy++)
        for (int dx = -r; dx <= r; dx++)
            if (dx*dx + dy*dy <= r*r)
                plot_pixel(cx+dx, cy+dy, color);
}

// layered ball:
void draw_ball(int x, int y, short int color) {
    // x,y is top-left of bounding box; center it
    int cx = x + BALL_SIZE/2;
    int cy = y + BALL_SIZE/2;
	
    if (color == BLACK) {
        // erase — just black out bounding box
        draw_rect(x-1, y-1, BALL_SIZE+2, BALL_SIZE+2, BLACK);
        return;
    }
	
    fill_circle(cx, cy, BALL_SIZE/2, COL_BALL_OUT);  // outer red
    fill_circle(cx, cy, BALL_SIZE/2 - 2, COL_BALL_MID); // orange mid
    fill_circle(cx, cy, BALL_SIZE/2 - 3, COL_BALL_IN); // bright
    
	// tiny specular dot top-left
    plot_pixel(cx-1, cy-2, 0xFFFF);
}

// layered target
void draw_target(int col, int row, short int color) {
    int cx = MAP_OFFSET_X + col * TILE + TILE/2;
    int cy = MAP_OFFSET_Y + row * TILE + TILE/2;
	
    if (color == BLACK) {
        draw_rect(cx - TILE/2, cy - TILE/2, TILE, TILE, BLACK);
        return;
    }
	
    draw_circle(cx, cy, 6, COL_TGT_PULSE);  // outer pulse ring
    fill_circle(cx, cy, 4, COL_TGT_OUT);  // cyan fill
    fill_circle(cx, cy, 2, COL_TGT_MID);  // green mid
	
    plot_pixel(cx, cy, COL_TGT_IN); // white center
    plot_pixel(cx+1, cy, COL_TGT_IN);
    plot_pixel(cx, cy+1, COL_TGT_IN);
    plot_pixel(cx+1, cy+1, COL_TGT_IN);
}

// draw full map 
void draw_map(int m) {
    // fill everything black
    for (int y = 0; y < SCREEN_H; y++)
        for (int x = 0; x < SCREEN_W; x++)
            Buffer1[y][x] = BLACK;

    // wall tiles
    for (int row = 0; row < ROWS; row++)
        for (int col = 0; col < COLS; col++)
            if (maps[m][row][col] == 1)
                draw_wall_tile(col, row);
}

// spawn target on random open tile
void spawn_target(int m, int bc, int br) {
    int col, row;
    do {
        col = 1 + rand() % (COLS-2);
        row = 1 + rand() % (ROWS-2);
    } while (maps[m][row][col] != 0 || (col==bc && row==br));
	
    target_col = col;
    target_row = row;
	
    draw_target(target_col, target_row, COL_TGT_OUT);
}

// collision
int hits_wall(int m, int px, int py) {
    int pts[4][2] = {
        {px, py},
        {px + BALL_SIZE - 1, py},
        {px, py + BALL_SIZE - 1},
        {px + BALL_SIZE - 1, py + BALL_SIZE - 1},
    };
	
    for (int i = 0; i < 4; i++) {
        int col = (pts[i][0] - MAP_OFFSET_X) / TILE;
        int row = (pts[i][1] - MAP_OFFSET_Y) / TILE;
        if (col < 0 || col >= COLS || row < 0 || row >= ROWS) return 1;
        if (maps[m][row][col] == 1) return 1;
    }
    return 0;
}

int reached_target(int px, int py) {
    int bc = (px + BALL_SIZE/2 - MAP_OFFSET_X) / TILE;
    int br = (py + BALL_SIZE/2 - MAP_OFFSET_Y) / TILE;
    return (bc == target_col && br == target_row);
}



// vsync + ps2
void wait_for_vsync(void) {
    volatile int *ctrl = (int *)0xFF203020;
    *ctrl = 1;
    while ((*(ctrl+3) & 0x01) != 0); //S bit turns back to zero
}

volatile int *ps2 = (volatile int *)0xFF200100;

int ps2_read() {
    int val = *ps2;
    if (val & 0x8000) return val & 0xFF;
    return -1;
}

// main
int main(void) {
    volatile int *pixel_ctrl = (int *)0xFF203020;
    *(pixel_ctrl+1) = (int)&Buffer1;
    *pixel_ctrl = 1;
    while ((*(pixel_ctrl+3) & 0x01) != 0);
    *(pixel_ctrl+1) = (int)&Buffer1;

    int cm = 0; //current map
    draw_map(cm);

    int px = MAP_OFFSET_X + TILE + (TILE-BALL_SIZE)/2;
    int py = MAP_OFFSET_Y + TILE + (TILE-BALL_SIZE)/2;
	
    draw_ball(px, py, COL_BALL_OUT);
    spawn_target(cm, 1, 1);

    int e0 = 0, skip = 0;

    while (1) {
        int b;
        while ((b = ps2_read()) >= 0) {
            if (skip) { 
				skip=0; 
				e0=0; 
				continue; 
			}
            if (b == 0xF0) { 
				skip=1; 
				continue; 
			}
            if (b == 0xE0) { 
				e0=1;   
				continue; 
			}

            int nx = px, ny = py;
			
            if (e0) {
                if (b==0x6B) nx -= SPEED;
                else if (b==0x74) nx += SPEED;
                else if (b==0x75) ny -= SPEED;
                else if (b==0x72) ny += SPEED;
                e0 = 0;
            } else {
                if (b==0x1D) ny -= SPEED; // W
                else if (b==0x1B) ny += SPEED; // S
                else if (b==0x1C) nx -= SPEED; // A
                else if (b==0x23) nx += SPEED; // D
            }

            if (!hits_wall(cm, nx, ny)) {
                draw_ball(px, py, BLACK);  // erase cleanly
                px = nx; py = ny;
				
                // redraw target if ball just left that area
                draw_target(target_col, target_row, COL_TGT_OUT);
                draw_ball(px, py, COL_BALL_OUT);
            }

			//check if target is hit
            if (reached_target(px, py)) {
                cm = rand() % NUM_MAPS; //random map of the 6 options
                draw_map(cm);
				
                px = MAP_OFFSET_X + TILE + (TILE-BALL_SIZE)/2;
                py = MAP_OFFSET_Y + TILE + (TILE-BALL_SIZE)/2;
				
                draw_ball(px, py, COL_BALL_OUT);
                spawn_target(cm, 1, 1);
            }
        }
    }
    return 0;
}

//******************

int abs(int x){
	if (x>0) {return x;} else {return (x*-1);}
}

void swap(int *x, int *y){
	int temp = *x;
	*x = *y;
	*y = temp;
}


void line(int x0, int y0, int x1, int y1, short color){
	
	int is_steep = abs(y1 - y0) > abs(x1 - x0); //used as bool
 	if (is_steep) {
		swap(&x0, &y0);
 		swap(&x1, &y1);
	}
 	if (x0 > x1){
		swap(&x0, &x1);
  		swap(&y0, &y1);
	} 

   	int dx = x1 - x0;
   	int dy = abs(y1 - y0);
   	int error = -(dx / 2);
   	int y = y0;
	
   	int y_step;
   	if (y0 < y1){y_step = 1;} else {y_step = -1;}
  
	for(int i = x0; i<=x1; i++) {
		if (is_steep) {
			plot_pixel(y, i, color);
		} else {
			plot_pixel(i, y, color);
		}
		error = error + dy;
		if (error>0) {
			y = y+y_step;
			error = error - dx;
		}
		
	}
	
	
}

void quad(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, short color) {
    //Vertices: (x0,y0), (x1,y1), (x2,y2), (x3,y3) in order (CW or CCW)
    int xs[4] = {x0, x1, x2, x3};
    int ys[4] = {y0, y1, y2, y3};

    //Find vertical bounds
    int ymin = ys[0], ymax = ys[0];
    for (int i = 1; i < 4; i++) {
        if (ys[i] < ymin) ymin = ys[i];
        if (ys[i] > ymax) ymax = ys[i];
    }

	if(((x0>160) && (y0>120)) || ((x0<160) && (y0<120))){
    	//Scanline fill
    	for (int y = ymin + 1; y < ymax; y++) {
        	int intersections[4];
        	int count = 0;

        	//Check each edge for intersection with this scanline
        	for (int i = 0; i < 4; i++) {
            	int ax = xs[i],       ay = ys[i];
            	int bx = xs[(i+1)%4], by = ys[(i+1)%4];

            	//Check if scanline y crosses this edge
            	if ((y > ay && y <= by) || (y > by && y <= ay)) {
                	//Compute x intersection using linear interpolation
                	int x_intersect = ax + (y - ay) * (bx - ax) / (by - ay);
                	intersections[count++] = x_intersect;
            	}
        	}

        	//Sort the (at most 2 for convex, up to 4 for concave) intersection points
        	for (int a = 0; a < count - 1; a++)
            	for (int b = a + 1; b < count; b++)
                	if (intersections[a] > intersections[b]) {
                    	int tmp = intersections[a];
                    	intersections[a] = intersections[b];
                    	intersections[b] = tmp;
                	}

        	//Fill between pairs of intersections
        	for (int a = 0; a < count - 1; a += 2)
            	for (int x = intersections[a] + 1; x < intersections[a+1]; x++)
                	plot_pixel(x, y, color);
    	}
	} else {
    	// Scanline fill (interior)
    	for (int y = ymin; y <= ymax; y++) {
        	int intersections[4];
        	int count = 0;
        	for (int i = 0; i < 4; i++) {
            	int ax = xs[i],       ay = ys[i];
            	int bx = xs[(i+1)%4], by = ys[(i+1)%4];
            	if ((y > ay && y <= by) || (y > by && y <= ay)) {
                	int x_intersect = ax + (y - ay) * (bx - ax) / (by - ay);
                	intersections[count++] = x_intersect;
            	}
        	}
        	for (int a = 0; a < count - 1; a++)
            	for (int b = a + 1; b < count; b++)
                	if (intersections[a] > intersections[b]) {
                    	int tmp = intersections[a];
                    	intersections[a] = intersections[b];
                    	intersections[b] = tmp;
                	}
        	for (int a = 0; a < count - 1; a += 2)
            	for (int x = intersections[a]; x <= intersections[a+1]; x++)
                	plot_pixel(x, y, color);
    	}
	}
		
}

struct TwoDPoint projectPoint(int x, int y, int z){
	struct TwoDPoint newP;
	int d = CAMERA_DISTANCE;
	newP.x = (-(x)*d)/(z - d)+160;
	newP.y = (-(y)*d)/(z - d)+120;
	return newP;
}

struct BoxPoints getBoxPoints(int x, int y, int z){
	struct BoxPoints newB;
	int w = HALF_BOX_WIDTH;
	newB.ftl = projectPoint(x-w, y+w, z+w);
	newB.ftr = projectPoint(x+w, y+w, z+w);
	newB.fbl = projectPoint(x-w, y-w, z+w);
	newB.fbr = projectPoint(x+w, y-w, z+w);
	newB.btl = projectPoint(x-w, y+w, z-w);
	newB.btr = projectPoint(x+w, y+w, z-w);
	newB.bbl = projectPoint(x-w, y-w, z-w);
	newB.bbr = projectPoint(x+w, y-w, z-w);
	return newB;
}

void drawBox(int x, int y, int z, short color){
	struct BoxPoints box = getBoxPoints(x,y,z);
	
	if (y<0)
    	quad(box.btl.x, box.btl.y, box.btr.x, box.btr.y, box.ftr.x, box.ftr.y, box.ftl.x, box.ftl.y, RED);
    if (x>0)
		quad(box.fbl.x, box.fbl.y, box.bbl.x, box.bbl.y, box.btl.x, box.btl.y,box.ftl.x, box.ftl.y,  GREEN);
	if (x<0)
    	quad(box.ftr.x, box.ftr.y, box.btr.x, box.btr.y, box.bbr.x, box.bbr.y, box.fbr.x, box.fbr.y, BLUE);
	if (y>0)
    	quad(box.fbl.x, box.fbl.y, box.fbr.x, box.fbr.y, box.bbr.x, box.bbr.y, box.bbl.x, box.bbl.y, ORANGE);
	
	//top face
	quad(box.ftl.x, box.ftl.y, box.ftr.x, box.ftr.y,box.fbr.x, box.fbr.y,box.fbl.x, box.fbl.y,COL_WALL_CORE);
	//Front face
    line(box.ftl.x, box.ftl.y, box.ftr.x, box.ftr.y, color);
    line(box.ftr.x, box.ftr.y, box.fbr.x, box.fbr.y, color);
    line(box.fbr.x, box.fbr.y, box.fbl.x, box.fbl.y, color);
    line(box.fbl.x, box.fbl.y, box.ftl.x, box.ftl.y, color);

    //Back face
	
    if(y<0) line(box.btl.x, box.btl.y, box.btr.x, box.btr.y, color);
    if(x<0) line(box.btr.x, box.btr.y, box.bbr.x, box.bbr.y, color);
    if(y>0) line(box.bbr.x, box.bbr.y, box.bbl.x, box.bbl.y, color);
    if(x>0) line(box.bbl.x, box.bbl.y, box.btl.x, box.btl.y, color);

    //front to back
    if((y<0)||(x>0))line(box.ftl.x, box.ftl.y, box.btl.x, box.btl.y, color);
    if((y<0)||(x<0))line(box.ftr.x, box.ftr.y, box.btr.x, box.btr.y, color);
    if((y>0)||(x>0))line(box.fbl.x, box.fbl.y, box.bbl.x, box.bbl.y, color);
    if((y>0)||(x<0))line(box.fbr.x, box.fbr.y, box.bbr.x, box.bbr.y, color);
	
}