#include <stdlib.h>

#define SCREEN_W   320
#define SCREEN_H   240
#define BLACK 0x0000
#define TILE 16
#define COLS 10
#define ROWS 10
#define NUM_MAPS 6
#define BALL_SIZE 8
#define SPEED 2

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
    {1,0,0,0,0,0,0,0,0,1},
    {1,0,0,1,0,0,0,1,0,1},
    {1,0,0,0,0,1,0,0,0,1},
    {1,0,1,0,0,0,0,0,0,1},
    {1,1,0,0,1,0,0,1,0,1},
    {1,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1},
},};

void plot_pixel(int x, int y, short int color) {
    if (x < 0 || x >= SCREEN_W || y < 0 || y >= SCREEN_H) return;
    Buffer1[y][x] = color;
}

void draw_rect(int x, int y, int w, int h, short int color) {
    for (int r = 0; r < h; r++)
        for (int c = 0; c < w; c++)
            plot_pixel(x+c, y+r, color);
}

// ── pretty wall tile ─────────────────────────────────────────────────────────
// dark outer border → solid white core → 1px shine top-left
void draw_wall_tile(int col, int row) {
    int x = MAP_OFFSET_X + col * TILE;
    int y = MAP_OFFSET_Y + row * TILE;

    // outer dark border 1px
    draw_rect(x, y, TILE, 1, COL_WALL_OUTER); // top
    draw_rect(x, y, 1, TILE, COL_WALL_OUTER); // left
    draw_rect(x, y+TILE-1, TILE, 1, 0x0000); // bottom shadow
    draw_rect(x+TILE-1, y, 1, TILE, 0x0000); // right shadow

    // white core (inset 1px)
    draw_rect(x+1, y+1, TILE-2, TILE-2, COL_WALL_CORE);

    // 1px shine line top and left inside
    draw_rect(x+1, y+1, TILE-2, 1, COL_WALL_SHINE);
    draw_rect(x+1, y+1, 1, TILE-2, COL_WALL_SHINE);
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