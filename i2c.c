// accel_dot.c
//
// Moves a dot on VGA based on MPU-9250 accelerometer tilt.
// No keyboard, no maze, no RL agent — just the dot.
//
// HOW I2C WORKS IN ONE SENTENCE:
//   You wiggle two wires (SCL=clock, SDA=data) in a specific pattern
//   to send/receive bytes to a chip. That's it. The functions below
//   do the wiggling in software (called "bit-banging").
//
// WIRING (JP1 header on DE1-SoC):
//   JP1 Pin 1  →  MPU VCC   (3.3V power)
//   JP1 Pin 2  →  MPU GND   (ground)
//   JP1 Pin 2  →  MPU AD0   (tie AD0 to GND, sets I2C address to 0x68)
//   JP1 Pin 3  →  MPU SCL   (clock wire)
//   JP1 Pin 4  →  MPU SDA   (data wire)
//
//   ALSO: put a 10k resistor between SCL and 3.3V
//         put a 10k resistor between SDA and 3.3V
//   (these "pull-up" resistors are required for I2C to work)
//   (your GY module might already have them — look for R1/R2 near the pins)

#include <stdlib.h>

// ─────────────────────────────────────────────────────────────────────────────
// VGA / screen constants
// ─────────────────────────────────────────────────────────────────────────────

#define SCREEN_W  320
#define SCREEN_H  240

// 16-bit colors in RGB565 format (5 bits red, 6 green, 5 blue)
#define BLACK   0x0000
#define WHITE   0xFFFF
#define CYAN    0x07FF
#define RED     0xF800

// pixel buffer — 512 wide because DE1-SoC needs power-of-2 row stride
short int Buffer1[240][512];

// ─────────────────────────────────────────────────────────────────────────────
// JP1 GPIO registers (from system.h — JP1_BASE = 0xFF200060)
// ─────────────────────────────────────────────────────────────────────────────

// Writing to jp1_data sets what value an OUTPUT pin drives (0 or 1)
// Writing to jp1_dir  sets whether each pin is INPUT(0) or OUTPUT(1)
//
// I2C is "open-drain": you NEVER drive a pin HIGH in software.
// To make a pin HIGH: set it as INPUT and let the pull-up resistor pull it up.
// To make a pin LOW:  set it as OUTPUT and write 0.
// This is safe because multiple devices share the same wires.

volatile int *jp1_data = (volatile int *)0xFF200060;
volatile int *jp1_dir  = (volatile int *)0xFF200064;

#define SCL_BIT  0   // JP1 GPIO bit 0 = SCL wire
#define SDA_BIT  1   // JP1 GPIO bit 1 = SDA wire

// ─────────────────────────────────────────────────────────────────────────────
// MPU-9250 I2C address and register numbers
// ─────────────────────────────────────────────────────────────────────────────

// The MPU's address on the I2C bus.
// AD0 pin tied to GND → address is 0x68
// AD0 pin tied to 3.3V → address would be 0x69
#define MPU_ADDR  0x68

// Register addresses inside the MPU-9250.
// These are just memory locations inside the chip.
// The full list is in the MPU-9250 datasheet.
#define REG_PWR_MGMT_1   0x6B   // power management — must write 0 to wake chip
#define REG_WHO_AM_I     0x75   // chip ID register — always returns 0x71
#define REG_ACCEL_XOUT_H 0x3B   // X acceleration high byte
#define REG_ACCEL_XOUT_L 0x3C   // X acceleration low byte
#define REG_ACCEL_YOUT_H 0x3D   // Y acceleration high byte
#define REG_ACCEL_YOUT_L 0x3E   // Y acceleration low byte

// ─────────────────────────────────────────────────────────────────────────────
// Sensitivity tuning
// ─────────────────────────────────────────────────────────────────────────────

// How many accelerometer counts = "full tilt" (maps to screen edge).
// Lower = more sensitive. Higher = need bigger tilt to reach edge.
//   6000 → sluggish, needs big tilts
//   4000 → natural, recommended
//   2000 → very twitchy, small tilt = full movement
#define TILT_SCALE  4000

// Dead zone: ignore readings smaller than this when board is flat.
// Prevents drift when sitting still on a table.
// Raise this if the dot creeps around when you're not touching it.
#define DEAD_ZONE   800

// ─────────────────────────────────────────────────────────────────────────────
// I2C bit-bang low level — pin wiggling
// ─────────────────────────────────────────────────────────────────────────────

// Small delay between pin changes.
// I2C needs the clock to stay stable long enough for the slave to sample it.
// If the dot behaves weirdly or WHO_AM_I returns garbage, increase to 500.
void i2c_delay() {
    volatile int i;
    for (i = 0; i < 100; i++);
}

// Release SCL — pull-up resistor pulls it HIGH
void scl_high() {
    *jp1_dir &= ~(1 << SCL_BIT);  // set as input (release)
    i2c_delay();
}

// Drive SCL LOW — we actively pull it down
void scl_low() {
    *jp1_data &= ~(1 << SCL_BIT); // make sure output register is 0
    *jp1_dir  |=  (1 << SCL_BIT); // set as output (now driving 0)
    i2c_delay();
}

// Release SDA — pull-up pulls it HIGH
void sda_high() {
    *jp1_dir &= ~(1 << SDA_BIT);
    i2c_delay();
}

// Drive SDA LOW
void sda_low() {
    *jp1_data &= ~(1 << SDA_BIT);
    *jp1_dir  |=  (1 << SDA_BIT);
    i2c_delay();
}

// Read what the slave is driving on SDA
// (we release the pin first so we don't fight the slave)
int sda_read() {
    *jp1_dir &= ~(1 << SDA_BIT);  // release so slave can drive it
    i2c_delay();
    return (*jp1_data >> SDA_BIT) & 1;
}

// ─────────────────────────────────────────────────────────────────────────────
// I2C protocol — START, STOP, send byte, receive byte
// ─────────────────────────────────────────────────────────────────────────────

// START condition: SDA drops LOW while SCL is HIGH.
// This special pattern signals "a transaction is beginning" to all devices.
void i2c_start() {
    sda_high();
    scl_high();
    sda_low();   // <-- SDA falls while SCL is high = START
    scl_low();   // bring SCL low ready for first bit
}

// STOP condition: SDA rises HIGH while SCL is HIGH.
// Signals "transaction is done, bus is free".
void i2c_stop() {
    sda_low();
    scl_high();
    sda_high();  // <-- SDA rises while SCL is high = STOP
}

// Send 8 bits, MSB first. After the 8th bit, the slave should pull SDA
// low to send an ACK (acknowledgement) meaning "I got it, keep going".
// Returns 0 if ACK received (good), 1 if NACK (something wrong).
int i2c_send_byte(unsigned char byte) {
    int i;
    // send each bit, most significant first
    for (i = 7; i >= 0; i--) {
        // put bit on SDA before clock pulse
        if ((byte >> i) & 1)
            sda_high();
        else
            sda_low();
        scl_high();  // slave reads SDA on rising edge of SCL
        scl_low();   // bring clock back low before next bit
    }
    // 9th clock: release SDA and let slave ACK
    sda_high();      // release SDA so slave can pull it low
    scl_high();
    int nack = sda_read();  // 0 = slave pulled low = ACK = good
    scl_low();
    return nack;     // 0=ACK(good)  1=NACK(bad)
}

// Read 8 bits from the slave.
// ack=1 means "send ACK after" (more bytes coming)
// ack=0 means "send NACK after" (this is the last byte, we're done)
unsigned char i2c_recv_byte(int ack) {
    unsigned char byte = 0;
    int i;
    sda_high();   // release SDA so slave can drive it
    for (i = 7; i >= 0; i--) {
        scl_high();                        // clock in a bit
        if (sda_read()) byte |= (1 << i); // sample SDA while SCL is high
        scl_low();
    }
    // tell slave if we want more bytes (ACK) or we're done (NACK)
    if (ack) sda_low(); else sda_high();
    scl_high();
    scl_low();
    sda_high();
    return byte;
}

// ─────────────────────────────────────────────────────────────────────────────
// MPU-9250 read/write using the I2C primitives above
// ─────────────────────────────────────────────────────────────────────────────

// Write one byte to a register inside the MPU.
// Pattern: START → address+W → register → value → STOP
void mpu_write_reg(unsigned char reg, unsigned char val) {
    i2c_start();
    i2c_send_byte(MPU_ADDR << 1);  // shift left 1, bit0=0 means WRITE
    i2c_send_byte(reg);             // which register to write
    i2c_send_byte(val);             // what value to write
    i2c_stop();
}

// Read one byte from a register inside the MPU.
// Pattern: START → address+W → register → RESTART → address+R → read byte → STOP
// The "RESTART" (repeated start) lets us switch from write to read mode
// without releasing the bus.
unsigned char mpu_read_reg(unsigned char reg) {
    i2c_start();
    i2c_send_byte(MPU_ADDR << 1);        // address + WRITE (to set register pointer)
    i2c_send_byte(reg);                   // which register we want to read
    i2c_start();                          // repeated START — switch to read mode
    i2c_send_byte((MPU_ADDR << 1) | 1);  // address + READ (bit0=1)
    unsigned char val = i2c_recv_byte(0); // read 1 byte, NACK = last byte
    i2c_stop();
    return val;
}

// ─────────────────────────────────────────────────────────────────────────────
// MPU-9250 init and data reading
// ─────────────────────────────────────────────────────────────────────────────

void mpu_init() {
    // Start with both SCL and SDA released (inputs, pulled high by resistors)
    *jp1_dir &= ~((1 << SCL_BIT) | (1 << SDA_BIT));

    // The MPU-9250 boots in SLEEP mode — you must wake it up.
    // Register 0x6B = PWR_MGMT_1. Writing 0x00 clears the sleep bit.
    mpu_write_reg(REG_PWR_MGMT_1, 0x00);

    // Small pause to let the chip stabilize after waking
    volatile int i;
    for (i = 0; i < 1000000; i++);
}

// Read X and Y axes and return as signed 16-bit values.
// Raw range: +16384 = 1g (one unit of gravity), -16384 = -1g
// When flat on a table: X≈0, Y≈0, Z≈+16384
void mpu_read_accel(short *ax, short *ay) {
    // Each axis is two bytes: HIGH byte and LOW byte.
    // Combine them into one 16-bit signed integer.
    unsigned char xh = mpu_read_reg(REG_ACCEL_XOUT_H);
    unsigned char xl = mpu_read_reg(REG_ACCEL_XOUT_L);
    unsigned char yh = mpu_read_reg(REG_ACCEL_YOUT_H);
    unsigned char yl = mpu_read_reg(REG_ACCEL_YOUT_L);
    *ax = (short)((xh << 8) | xl);  // merge high and low bytes
    *ay = (short)((yh << 8) | yl);
}

// ─────────────────────────────────────────────────────────────────────────────
// VGA drawing
// ─────────────────────────────────────────────────────────────────────────────

void plot_pixel(int x, int y, short color) {
    if (x < 0 || x >= SCREEN_W || y < 0 || y >= SCREEN_H) return;
    Buffer1[y][x] = color;
}

void fill_circle(int cx, int cy, int r, short color) {
    int dx, dy;
    for (dy = -r; dy <= r; dy++)
        for (dx = -r; dx <= r; dx++)
            if (dx*dx + dy*dy <= r*r)
                plot_pixel(cx+dx, cy+dy, color);
}

void clear_screen() {
    int x, y;
    for (y = 0; y < SCREEN_H; y++)
        for (x = 0; x < SCREEN_W; x++)
            Buffer1[y][x] = BLACK;
}

void wait_for_vsync() {
    volatile int *ctrl = (volatile int *)0xFF203020;
    *ctrl = 1;
    while ((*(ctrl + 3) & 0x01) != 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// Main
// ─────────────────────────────────────────────────────────────────────────────

int main(void) {
    // Point the VGA controller at our pixel buffer
    volatile int *pixel_ctrl = (volatile int *)0xFF203020;
    *(pixel_ctrl + 1) = (int)Buffer1;
    *pixel_ctrl = 1;
    while ((*(pixel_ctrl + 3) & 0x01) != 0);
    *(pixel_ctrl + 1) = (int)Buffer1;

    clear_screen();
    mpu_init();

    // dot starts in the center of the screen
    int dot_x = 160, dot_y = 120;
    int old_x = dot_x, old_y = dot_y;

    short ax, ay;

    while (1) {
        // 1. read accelerometer
        mpu_read_accel(&ax, &ay);

        // 2. erase old dot position
        fill_circle(old_x, old_y, 8, BLACK);

        // 3. apply dead zone — ignore tiny noise when board is flat
        int ix = (int)ax;
        int iy = (int)ay;
        if (ix > -DEAD_ZONE && ix < DEAD_ZONE) ix = 0;
        if (iy > -DEAD_ZONE && iy < DEAD_ZONE) iy = 0;

        // 4. convert tilt to screen position
        //    TILT_SCALE counts = full screen width/height of movement
        //    negate iy because tilting forward = positive Y on chip but up on screen
        dot_x = 160 + (ix * 150) / TILT_SCALE;
        dot_y = 120 - (iy * 110) / TILT_SCALE;

        // 5. clamp so dot stays on screen
        if (dot_x <   8) dot_x =   8;
        if (dot_x > 311) dot_x = 311;
        if (dot_y <   8) dot_y =   8;
        if (dot_y > 231) dot_y = 231;

        // 6. draw dot at new position
        fill_circle(dot_x, dot_y, 8, CYAN);

        old_x = dot_x;
        old_y = dot_y;

        wait_for_vsync();
    }

    return 0;
}