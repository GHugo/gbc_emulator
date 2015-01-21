#ifndef __GPU_H__
#define __GPU_H__

#include "memory.h"
#include <SDL/SDL.h>

#define SCREEN_WIDTH 160
#define SCREEN_HEIGHT 144
#define MAP_LINE_WIDTH 32
#define MAP_LINE_HEIGHT 32
#define TILE_WIDTH 8
#define TILE_HEIGHT 8

#define TILE_ENCODED_SIZE (sizeof(uint16_t))

#define MAP_TOTAL_WIDTH (MAP_LINE_WIDTH * TILE_WIDTH)
#define MAP_TOTAL_HEIGHT (MAP_LINE_HEIGHT * TILE_HEIGHT)

typedef enum {
	GPU_HORIZ_BLANK,
	GPU_VERT_BLANK,
	GPU_SCAN_OAM,
	GPU_SCAN_VRAM
} gpu_mode;

typedef enum {
	GPU_HORIZ_BLANK_TIMING = 204,
	GPU_VERT_BLANK_TIMING  = 4560,
	GPU_SCAN_OAM_TIMING    = 80,
	GPU_SCAN_VRAM_TIMING   = 172
} gpu_timing;


typedef struct {
	SDL_Surface *surface;
	uint16_t state_start_clock;
	gpu_mode mode;
	uint8_t cur_line;
	uint8_t* vram;
	uint8_t scroll_x;
	uint8_t scroll_y;
	uint8_t pal;
	uint8_t tile_set;
	uint8_t cur_map;
} gpu;

gpu* gpu_init(memory *mem);
void gpu_end(gpu* gp);

void gpu_process(gpu *gp, uint16_t clock);
#endif     // __GPU_H__
