#include <stdlib.h>

#define SCREEN_W   320
#define SCREEN_H   240
#define BLACK      0x0000
#define RED        0xF800
#define WHITE      0xFFFF
#define GREEN      0x07E0
#define CYAN       0x07FF
#define YELLOW     0xFFE0

#define TILE       16    // bigger tiles since maps are small
#define COLS       10
#define ROWS       10
#define NUM_MAPS   6
#define BALL_SIZE  6
#define SPEED      2

// Each map is 10x10 tiles = 160x160 pixels, centered on screen
#define MAP_OFFSET_X  ((SCREEN_W - COLS*TILE) / 2)
#define MAP_OFFSET_Y  ((SCREEN_H - ROWS*TILE) / 2)

short int Buffer1[240][512];

// target tile position (in tile coords)
int target_col, target_row;

// ── MAP 0: simple box with inner cross ──────────────────────────────────────
int maps[NUM_MAPS][ROWS][COLS] = {
{
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
},
// ── MAP 1: zigzag corridors ──────────────────────────────────────────────────
{
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
},
// ── MAP 2: ring with holes ───────────────────────────────────────────────────
{
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
},
// ── MAP 3: diagonal staircase chunks ────────────────────────────────────────
{
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
},
// ── MAP 4: four small rooms, narrow doorways ─────────────────────────────────
{
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
},
// ── MAP 5: open field with scattered pillars ─────────────────────────────────
{
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
},
};

// ── drawing helpers ──────────────────────────────────────────────────────────

void plot_pixel(int x, int y, short int color) {
    if (x < 0 || x >= SCREEN_W || y < 0 || y >= SCREEN_H) return;
    Buffer1[y][x] = color;
}

void draw_rect(int x, int y, int w, int h, short int color) {
    for (int r = 0; r < h; r++)
        for (int c = 0; c < w; c++)
            plot_pixel(x + c, y + r, color);
}

// draw hollow circle for target
void draw_circle(int cx, int cy, int radius, short int color) {
    int x = 0, y = radius, d = 1 - radius;
    while (x <= y) {
        plot_pixel(cx+x, cy+y, color); plot_pixel(cx-x, cy+y, color);
        plot_pixel(cx+x, cy-y, color); plot_pixel(cx-x, cy-y, color);
        plot_pixel(cx+y, cy+x, color); plot_pixel(cx-y, cy+x, color);
        plot_pixel(cx+y, cy-x, color); plot_pixel(cx-y, cy-x, color);
        if (d < 0) d += 2*x + 3;
        else { d += 2*(x-y) + 5; y--; }
        x++;
    }
}

void draw_target(int col, int row, short int color) {
    int cx = MAP_OFFSET_X + col * TILE + TILE/2;
    int cy = MAP_OFFSET_Y + row * TILE + TILE/2;
    draw_circle(cx, cy, 5, color);
    draw_circle(cx, cy, 3, color);
    plot_pixel(cx, cy, color);
}

void draw_ball(int x, int y, short int color) {
    for (int r = 0; r < BALL_SIZE; r++)
        for (int c = 0; c < BALL_SIZE; c++)
            plot_pixel(x + c, y + r, color);
}

void draw_map(int m) {
    // clear screen
    for (int y = 0; y < SCREEN_H; y++)
        for (int x = 0; x < SCREEN_W; x++)
            Buffer1[y][x] = BLACK;

    // draw border background slightly different so map stands out
    draw_rect(MAP_OFFSET_X - 1, MAP_OFFSET_Y - 1,
              COLS * TILE + 2, ROWS * TILE + 2, 0x2104);

    // draw tiles
    for (int row = 0; row < ROWS; row++)
        for (int col = 0; col < COLS; col++)
            if (maps[m][row][col] == 1)
                draw_rect(MAP_OFFSET_X + col*TILE,
                          MAP_OFFSET_Y + row*TILE,
                          TILE, TILE, WHITE);
}

// pick a random open tile for the target (not the ball start tile)
void spawn_target(int m, int ball_col, int ball_row) {
    int col, row;
    do {
        col = 1 + rand() % (COLS - 2);
        row = 1 + rand() % (ROWS - 2);
    } while (maps[m][row][col] != 0 ||
             (col == ball_col && row == ball_row));
    target_col = col;
    target_row = row;
    draw_target(target_col, target_row, YELLOW);
}

// ── collision ────────────────────────────────────────────────────────────────

int hits_wall(int m, int px, int py) {
    // check all 4 corners of ball in screen space → tile space
    int corners[4][2] = {
        {px,              py},
        {px + BALL_SIZE-1, py},
        {px,              py + BALL_SIZE-1},
        {px + BALL_SIZE-1, py + BALL_SIZE-1},
    };
    for (int i = 0; i < 4; i++) {
        int col = (corners[i][0] - MAP_OFFSET_X) / TILE;
        int row = (corners[i][1] - MAP_OFFSET_Y) / TILE;
        if (col < 0 || col >= COLS || row < 0 || row >= ROWS) return 1;
        if (maps[m][row][col] == 1) return 1;
    }
    return 0;
}

int reached_target(int px, int py) {
    int ball_col = (px + BALL_SIZE/2 - MAP_OFFSET_X) / TILE;
    int ball_row = (py + BALL_SIZE/2 - MAP_OFFSET_Y) / TILE;
    return (ball_col == target_col && ball_row == target_row);
}

// ── vsync + ps2 ──────────────────────────────────────────────────────────────

void wait_for_vsync(void) {
    volatile int *ctrl = (int *)0xFF203020;
    *ctrl = 1;
    while ((*(ctrl + 3) & 0x01) != 0);
}

volatile int *ps2 = (volatile int *)0xFF200100;
int ps2_read() {
    int val = *ps2;
    if (val & 0x8000) return val & 0xFF;
    return -1;
}

// ── main ─────────────────────────────────────────────────────────────────────

int main(void) {
    volatile int *pixel_ctrl = (int *)0xFF203020;

    // single buffer mode
    *(pixel_ctrl + 1) = (int)&Buffer1;
    *pixel_ctrl = 1;
    while ((*(pixel_ctrl + 3) & 0x01) != 0);
    *(pixel_ctrl + 1) = (int)&Buffer1;

    int current_map = 0;  // start with map 0, swap to rand after first clear

    draw_map(current_map);

    // ball starts at tile (1,1) in map space
    int start_x = MAP_OFFSET_X + 1 * TILE + (TILE - BALL_SIZE) / 2;
    int start_y = MAP_OFFSET_Y + 1 * TILE + (TILE - BALL_SIZE) / 2;
    int px = start_x, py = start_y;
    draw_ball(px, py, RED);

    spawn_target(current_map, 1, 1);

    int e0 = 0, skip = 0;

    while (1) {
        int b;
        while ((b = ps2_read()) >= 0) {
            if (skip)      { skip = 0; e0 = 0; continue; }
            if (b == 0xF0) { skip = 1; continue; }
            if (b == 0xE0) { e0 = 1;  continue; }

            int nx = px, ny = py;

            if (e0) {
                if      (b == 0x6B) nx -= SPEED;
                else if (b == 0x74) nx += SPEED;
                else if (b == 0x75) ny -= SPEED;
                else if (b == 0x72) ny += SPEED;
                e0 = 0;
            } else {
                if      (b == 0x1D) ny -= SPEED; // W
                else if (b == 0x1B) ny += SPEED; // S
                else if (b == 0x1C) nx -= SPEED; // A
                else if (b == 0x23) nx += SPEED; // D
            }

            if (!hits_wall(current_map, nx, ny)) {
                draw_ball(px, py, BLACK);
                px = nx; py = ny;
                draw_ball(px, py, RED);
            }

            // reached target — load a new random map
            if (reached_target(px, py)) {
                current_map = rand() % NUM_MAPS;
                draw_map(current_map);
                px = MAP_OFFSET_X + 1 * TILE + (TILE - BALL_SIZE) / 2;
                py = MAP_OFFSET_Y + 1 * TILE + (TILE - BALL_SIZE) / 2;
                draw_ball(px, py, RED);
                spawn_target(current_map, 1, 1);
            }
        }
    }

    return 0;
}