#include "pulsesensor.h"
#include "gpio.h"
#include "uart.h"
#include "printf.h"
#include "timer.h"
#include "strings.h"
#include "mcp3008.h"
#include "console.h"
#include "gl.h"
#include "fb.h"
#include "display.h"
#include "font.h"
#include "ringbuffer.h"
#include "assert.h"
#include "interrupts.h"
#include "gpioextra.h"

// defining variables to make graphics extendable to any resolution.
#define _WIDTH 640
#define _HEIGHT 512
#define TEXT_X_OFFSET (_WIDTH - 120)
#define TEXT_Y_OFFSET 20
#define GRAPH_X_ORIGIN (_WIDTH / 10)
#define GRAPH_Y_ORIGIN (_HEIGHT / 10)
#define GRAPH_AXIS_THICKNESS (_WIDTH / 100)
#define GRAPH_AXIS_LENGTH (_HEIGHT / 1.2)
#define BACKGROUND_COLOR GL_WHITE
#define GRAPH_WIDTH_ADJUSTMENT ((_WIDTH - _HEIGHT) / 2)
#define PULSE_DATA_MAX 974
#define HEART_X (TEXT_X_OFFSET - 40)
#define HEART_Y (TEXT_Y_OFFSET + 5)
#define HEART_SIZE 30
#define FAST_HEART HEART_SIZE * .4
#define MEDIUM_HEART HEART_SIZE * .6
#define SLOW_HEART HEART_SIZE * .8

extern volatile rb_t* rb;

int* buf;
const unsigned int DATA = GPIO_PIN9;
static int buf_count;

void clear_graph();
static void graphics_init(int curr_heart_size);
static void draw_line_thick(int x, int y, int w, int h);
void draw_clover(int x, int y, int length, color_t c);
void draw_vshape(int x, int y, int length, color_t c);

void display_init() {
	buf_count = 0;
    gl_init(_WIDTH, _HEIGHT, GL_DOUBLEBUFFER);
    graphics_init(HEART_SIZE);
    graphics_init(SLOW_HEART); //call a second time to mirror image on both buffers.
}

// draws two lines to get basic graph setup
void draw_graph(int x, int y, int w, int h) {
	draw_line_thick(x, y, w, h);
	draw_line_thick(x, y + h, h + GRAPH_WIDTH_ADJUSTMENT, w);
}

// takes in bpm from pulsesensor and converts the int into a string. also draws bpm to buffer
void update_bpm(int bpm) {
	char string_bpm[3]; //create buffer of max 3 string digits
	snprintf(string_bpm, 4, "%d", bpm); //convert bpm int into string
	gl_draw_string(TEXT_X_OFFSET + (gl_get_char_width() * 4), TEXT_Y_OFFSET, string_bpm, GL_BLACK);
}

// clears the bpm count by covering digits with white rectangle.
void clear_bpm() {
	gl_draw_rect(TEXT_X_OFFSET + (gl_get_char_width() * 4), TEXT_Y_OFFSET,
								 (gl_get_char_width() * 3), gl_get_char_height(), BACKGROUND_COLOR);
}

// writes data points and lines on graph
void write_graph(int bpm) {
    static int bpm_count = 0;
    static int prev_pixel_x = 0; // keeps track of previous pixel position
    static int prev_pixel_y = 0;
    interrupts_global_disable();
	rb_dequeue(rb, &buf[buf_count]);
	gl_swap_buffer();
    double pixel_x = GRAPH_X_ORIGIN + GRAPH_AXIS_THICKNESS + buf_count;
    double pixel_y = GRAPH_Y_ORIGIN + GRAPH_AXIS_LENGTH - 
                    ((double)buf[buf_count] / (double)PULSE_DATA_MAX * (double)get_graph_height());
	gl_draw_pixel(pixel_x, pixel_y, GL_RED); // draws pixel to both buffers
	gl_swap_buffer();
	gl_draw_pixel(pixel_x, pixel_y, GL_RED);
    if (buf_count == 1) { // keep track of first few pixels in anticipation of drawing a line
       prev_pixel_x = pixel_x;
       prev_pixel_y = pixel_y;
    } else if (buf_count > 1) { // after first two pixels, starts to connect lines. Here, we also update bpm to reduce the amount of times we swap buffer.
        gl_draw_line(prev_pixel_x, prev_pixel_y, pixel_x, pixel_y, GL_RED);
        update_bpm(bpm);
        gl_swap_buffer();
        clear_bpm();
        gl_draw_line(prev_pixel_x, prev_pixel_y, pixel_x, pixel_y, GL_RED);
        prev_pixel_x = pixel_x; // reset previous pixel variables to the current ones.
        prev_pixel_y = pixel_y;
    }
    buf_count++;
    if (buf_count >= (GRAPH_AXIS_LENGTH - GRAPH_AXIS_THICKNESS + GRAPH_WIDTH_ADJUSTMENT)) {
        clear_graph(); // clear graph when data is written beyond the x axis.
        gl_swap_buffer();
        clear_graph();
        buf_count = 0;
    }
    interrupts_global_enable();
}

// parameters passed in are the center of the heart and width of heart
void display_heart(int x, int y, int width, color_t c) { 
    // a heart can be formed from 20 consecutive right triangles!
    int triangle_length = width / 4;

    // draw vertices of the heart
    draw_clover(x - triangle_length, y - triangle_length, triangle_length, c);
    draw_clover(x + triangle_length, y - triangle_length, triangle_length, c);
    draw_clover(x, y + triangle_length, triangle_length, c);

    // fill in the body
    draw_vshape(x - triangle_length, y, triangle_length, c);
    draw_vshape(x + triangle_length, y, triangle_length, c);
}

void draw_clover(int x, int y, int length, color_t c) {
    gl_draw_triangle(x, y, x - length, y, x, y - length, c);
    gl_draw_triangle(x, y, x + length, y, x, y - length, c);
    gl_draw_triangle(x, y, x - length, y, x, y + length, c);
    gl_draw_triangle(x, y, x + length, y, x, y + length, c);
}

void draw_vshape(int x, int y, int length, color_t c) {
    gl_draw_triangle(x, y, x - length, y, x - length, y - length, c);
    gl_draw_triangle(x, y, x + length, y, x + length, y - length, c);
    gl_draw_triangle(x, y, x - length, y, x, y + length, c);
    gl_draw_triangle(x, y, x + length, y, x, y + length, c);
}

// here, we will clear the heart if and only if the furthest left pixel of the normal
// sized heart is not red. This indicates to us that it is not the regular heart size.
// we only want to change the heart size of the heart that is NOT the regular sized one.
// for example, we start with a heart approx 30 pixels in width, and another heart approx.
// 24 pixels in width. We only want to change the heart with 24 pixels in width because 
// otherwise we'd lose the appearance of a pulsing heart between a normal sized heart (30 pixels)
// and an smaller sized heart.
void pulsate_heart(int bpm) {
    if (gl_read_pixel(HEART_X - 14, HEART_Y - 15) == GL_RED) { 
        clear_heart();
    }
    if (bpm < 75) {
        timer_delay_ms(400); //slow heartbeat
        display_heart(HEART_X, HEART_Y, SLOW_HEART, GL_RED);
    } else if (bpm > 75 && bpm < 110) {
        timer_delay_ms(250); //medium heartbeat
        display_heart(HEART_X, HEART_Y, MEDIUM_HEART, GL_RED);
    } else if (bpm > 110) {
        timer_delay_ms(100); //fast heartbeat
        display_heart(HEART_X, HEART_Y, FAST_HEART, GL_RED);
    }
}

int get_graph_height() {
	return (GRAPH_AXIS_LENGTH - GRAPH_AXIS_THICKNESS);
}

int get_graph_width() {
	return (GRAPH_AXIS_LENGTH + GRAPH_WIDTH_ADJUSTMENT - GRAPH_AXIS_THICKNESS);
}

// draws line, starting from x, y, to a, b, with a given thickness.
static void draw_line_thick(int x, int y, int w, int h) {
    gl_draw_rect(x, y, w, h, GL_BLACK);
}

static void graphics_init(int curr_heart_size) {
	gl_clear(BACKGROUND_COLOR); // Background should be white.
    gl_draw_string(TEXT_X_OFFSET, TEXT_Y_OFFSET, "BPM:", GL_RED);
    draw_graph(GRAPH_X_ORIGIN, GRAPH_Y_ORIGIN, GRAPH_AXIS_THICKNESS, GRAPH_AXIS_LENGTH);

    display_heart(HEART_X, HEART_Y, curr_heart_size, GL_RED); 
    gl_swap_buffer();
}

void clear_graph() {
    gl_draw_rect(GRAPH_X_ORIGIN + GRAPH_AXIS_THICKNESS, GRAPH_Y_ORIGIN,
     GRAPH_AXIS_LENGTH - GRAPH_AXIS_THICKNESS + GRAPH_WIDTH_ADJUSTMENT + 1, 
     GRAPH_AXIS_LENGTH, BACKGROUND_COLOR);
}

void clear_heart() {
    gl_draw_rect(HEART_X - 15, HEART_Y - 15, HEART_SIZE, HEART_SIZE, BACKGROUND_COLOR);
}


