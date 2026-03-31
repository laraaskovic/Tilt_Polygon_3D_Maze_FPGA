#include <stdint.h>

#define SCREEN_W 320
#define SCREEN_H 240
short int Buffer1[240][512];

typedef struct { volatile unsigned int DDR; volatile unsigned int DATA; } GPIO_t;

#define GPIO_BASE ((GPIO_t *)0xFF200060)
#define SCL_PIN 0
#define SDA_PIN 1

#define MPU_ADDR      0x68
#define REG_PWR_MGMT  0x6B
#define REG_ACCEL_X_H 0x3B

#define SENSITIVITY 1000
#define DEADZONE    200

void plot_pixel(int x, int y, short color) {
    if (x < 0 || x >= SCREEN_W || y < 0 || y >= SCREEN_H) return;
    Buffer1[y][x] = color;
}

void dbg_color(short color) {
    for (int dy = 0; dy < 6; dy++)
        for (int dx = 0; dx < 6; dx++)
            plot_pixel(2 + dx, 2 + dy, color);
}

void wait_for_vsync(void) {
    volatile int *ctrl = (int *)0xFF203020;
    *ctrl = 1;
    while ((*(ctrl + 3) & 0x01) != 0);
}

void clear_screen(void) {
    for (int y = 0; y < SCREEN_H; y++)
        for (int x = 0; x < SCREEN_W; x++)
            Buffer1[y][x] = 0x0000;
}

void draw_dot(int x, int y, short color) {
    int r = 5;
    for (int dy = -r; dy <= r; dy++)
        for (int dx = -r; dx <= r; dx++)
            if (dx*dx + dy*dy <= r*r)
                plot_pixel(x + dx, y + dy, color);
}

// draws a bar showing raw ax and ay values so you can see what the sensor reads
void draw_debug_bar(short ax, short ay) {
    // clear bar area
    for (int y = 20; y < 36; y++)
        for (int x = 0; x < SCREEN_W; x++)
            plot_pixel(x, y, 0x0000);

    int cx = SCREEN_W / 2;

    // ax bar — cyan, top row
    int bar_ax = cx + (int)(ax / 100);
    if (bar_ax < 0) bar_ax = 0;
    if (bar_ax >= SCREEN_W) bar_ax = SCREEN_W - 1;
    int x = cx;
    while (x != bar_ax) {
        for (int y = 20; y < 28; y++) plot_pixel(x, y, 0x07FF);
        x += (bar_ax > cx ? 1 : -1);
    }

    // ay bar — magenta, bottom row
    int bar_ay = cx + (int)(ay / 100);
    if (bar_ay < 0) bar_ay = 0;
    if (bar_ay >= SCREEN_W) bar_ay = SCREEN_W - 1;
    x = cx;
    while (x != bar_ay) {
        for (int y = 28; y < 36; y++) plot_pixel(x, y, 0xF81F);
        x += (bar_ay > cx ? 1 : -1);
    }
}

// ── I2C ──────────────────────────────────────────────────────────────────────
void sda_high(GPIO_t *g) { g->DDR &= ~(1 << SDA_PIN); }
void sda_low (GPIO_t *g) { g->DDR |=  (1 << SDA_PIN); g->DATA &= ~(1 << SDA_PIN); }
void scl_high(GPIO_t *g) { g->DDR |=  (1 << SCL_PIN); g->DATA |=  (1 << SCL_PIN); }
void scl_low (GPIO_t *g) { g->DDR |=  (1 << SCL_PIN); g->DATA &= ~(1 << SCL_PIN); }
int  sda_read(GPIO_t *g) { g->DDR &= ~(1 << SDA_PIN); return (g->DATA >> SDA_PIN) & 1; }

void i2c_delay(void) { volatile int i; for (i = 0; i < 200; i++); }

void i2c_start(GPIO_t *g) {
    sda_high(g); scl_high(g); i2c_delay();
    sda_low(g);               i2c_delay();
    scl_low(g);               i2c_delay();
}

void i2c_stop(GPIO_t *g) {
    sda_low(g);  scl_high(g); i2c_delay();
    sda_high(g);              i2c_delay();
}

int i2c_write_byte(GPIO_t *g, unsigned char byte) {
    for (int i = 7; i >= 0; i--) {
        if ((byte >> i) & 1) sda_high(g);
        else                 sda_low(g);
        i2c_delay();
        scl_high(g); i2c_delay();
        scl_low(g);  i2c_delay();
    }
    sda_high(g);
    scl_high(g); i2c_delay();
    int nack = sda_read(g);
    scl_low(g);  i2c_delay();
    return nack;
}

unsigned char i2c_read_byte(GPIO_t *g, int ack) {
    unsigned char byte = 0;
    sda_high(g);
    for (int i = 7; i >= 0; i--) {
        scl_high(g); i2c_delay();
        if (sda_read(g)) byte |= (1 << i);
        scl_low(g);  i2c_delay();
    }
    if (ack) sda_low(g);
    else     sda_high(g);
    scl_high(g); i2c_delay();
    scl_low(g);  i2c_delay();
    sda_high(g);
    return byte;
}

void mpu_write_reg(GPIO_t *g, unsigned char reg, unsigned char val) {
    i2c_start(g);
    i2c_write_byte(g, MPU_ADDR << 1);
    i2c_write_byte(g, reg);
    i2c_write_byte(g, val);
    i2c_stop(g);
}

void mpu_read_regs(GPIO_t *g, unsigned char reg, unsigned char *buf, int len) {
    i2c_start(g);
    i2c_write_byte(g, MPU_ADDR << 1);
    i2c_write_byte(g, reg);
    i2c_start(g);
    i2c_write_byte(g, (MPU_ADDR << 1) | 1);
    for (int i = 0; i < len; i++)
        buf[i] = i2c_read_byte(g, i < len - 1);
    i2c_stop(g);
}

void mpu_init(GPIO_t *g) {
    // ping the MPU first to check wiring
    i2c_start(g);
    int nack = i2c_write_byte(g, MPU_ADDR << 1);
    i2c_stop(g);

    if (nack) {
        dbg_color(0xF800);  // red — not found, check wiring
    } else {
        dbg_color(0x07FF);  // cyan — found
        mpu_write_reg(g, REG_PWR_MGMT, 0x00);  // wake up
    }
}

// ── main ─────────────────────────────────────────────────────────────────────
int main(void) {
    volatile int *pixel_ctrl = (int *)0xFF203020;
    *(pixel_ctrl + 1) = (int)&Buffer1;
    *pixel_ctrl = 1;
    while ((*(pixel_ctrl + 3) & 0x01) != 0);
    *(pixel_ctrl + 1) = (int)&Buffer1;

    GPIO_t *gpio = GPIO_BASE;

    clear_screen();
    mpu_init(gpio);
    wait_for_vsync();

    int dot_x = SCREEN_W / 2;
    int dot_y = SCREEN_H / 2;

    while (1) {
        unsigned char buf[4];
        mpu_read_regs(gpio, REG_ACCEL_X_H, buf, 4);

        short ax = (short)((buf[0] << 8) | buf[1]);
        short ay = (short)((buf[2] << 8) | buf[3]);

        // show raw values as bars — if these don't move when you tilt,
        // the sensor data is wrong (swap pins or check address)
        draw_debug_bar(ax, ay);

        // erase old dot
        draw_dot(dot_x, dot_y, 0x0000);

        // move dot — tune SENSITIVITY and DEADZONE if needed
        if (ax >  DEADZONE) dot_x += (ax / SENSITIVITY) + 1;
        if (ax < -DEADZONE) dot_x -= ((-ax) / SENSITIVITY) + 1;
        if (ay >  DEADZONE) dot_y -= (ay / SENSITIVITY) + 1;
        if (ay < -DEADZONE) dot_y += ((-ay) / SENSITIVITY) + 1;

        // clamp to screen
        if (dot_x < 5)           dot_x = 5;
        if (dot_x >= SCREEN_W-5) dot_x = SCREEN_W - 6;
        if (dot_y < 5)           dot_y = 5;
        if (dot_y >= SCREEN_H-5) dot_y = SCREEN_H - 6;

        // draw dot white
        draw_dot(dot_x, dot_y, 0xFFFF);

        // green = read completed fine
        dbg_color(0x07E0);

        wait_for_vsync();
    }

    return 0;
}