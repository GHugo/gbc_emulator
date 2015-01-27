#ifndef __GPU_H__
#define __GPU_H__

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

#define SPRITE_COUNT 40
#define SPRITE_HEIGHT 8
#define SPRITE_WIDTH 8

typedef enum {
	GPU_HORIZ_BLANK,
	GPU_VERT_BLANK,
	GPU_SCAN_OAM,
	GPU_SCAN_VRAM
} gpu_mode;

typedef enum {
	GPU_HORIZ_BLANK_TIMING = 51,
	GPU_VERT_BLANK_TIMING  = 1140,
	GPU_SCAN_OAM_TIMING    = 20,
	GPU_SCAN_VRAM_TIMING   = 43
} gpu_timing;

typedef struct {
	uint8_t y;
	uint8_t x;
	uint8_t tile;
	uint8_t options;
} oam_data;

typedef struct memory memory;

typedef struct gpu {
	SDL_Surface *surface;
	uint16_t state_start_clock;
	gpu_mode mode;
	uint8_t* vram;
	uint8_t* oam;

	struct {
		uint8_t control;
		uint8_t status;
		uint8_t cur_line;
		uint8_t scroll_x;
		uint8_t scroll_y;
		uint8_t bg_pal;
		uint8_t sp_pal_0;
		uint8_t sp_pal_1;
	} reg;
} gpu;

gpu* gpu_init(memory *mem);
void gpu_end(gpu* gp);
void gpu_process(gpu *gp, uint16_t clock);
#endif     // __GPU_H__
