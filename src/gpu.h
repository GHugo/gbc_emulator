#ifndef __GPU_H__
#define __GPU_H__

#include "memory.h"
#include <SDL/SDL.h>

#define SCREEN_WIDTH 160
#define SCREEN_HEIGHT 144

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
	uint32_t state_start_clock;
	gpu_mode mode;
	uint8_t cur_line;
} gpu;

gpu* gpu_init(memory *mem);
void gpu_end(gpu* gp);

void gpu_process(gpu *gp, uint32_t clock);
#endif     // __GPU_H__
