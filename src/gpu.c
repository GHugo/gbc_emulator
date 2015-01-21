#include <SDL/SDL.h>
#include <assert.h>
#include "log.h"
#include "gpu.h"

gpu* gpu_init(memory *mem) {
	gpu* gp = malloc(sizeof(gpu));
	if (gp == NULL)
		ERROR("Unable to allocate memory for gpu.\n");

	if (SDL_Init(SDL_INIT_VIDEO) == -1)
		ERROR("Unable to load SDL: %s\n", SDL_GetError());

	gp->surface = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, 32, SDL_HWSURFACE | SDL_DOUBLEBUF);
	if (gp->surface == NULL)
		ERROR("Unable to get the SDL surface: %s\n", SDL_GetError());

	// Fill with white
	if (SDL_FillRect(gp->surface, NULL, SDL_MapRGB(gp->surface->format, 0xFF, 0xFF, 0xFF)) == -1)
		ERROR("Unable to fill surface with white: %s\n", SDL_GetError());

	if (SDL_Flip(gp->surface) == -1)
		ERROR("Unable to flip surface at init: %s\n", SDL_GetError());

	gp->state_start_clock = 0;
	gp->cur_line = 0;
	gp->mode = GPU_HORIZ_BLANK;

	return gp;
}

void gpu_end(gpu *gp) {
	SDL_Quit();
	free(gp);
}

static void gpu_render(gpu *gp) {
}

// Timing from http://imrannazar.com/GameBoy-Emulation-in-JavaScript:-GPU-Timings
// TODO: simplify all this & only call SDL_Flip when needed
void gpu_process(gpu* gp, uint32_t clock) {
	switch(gp->mode) {
	case GPU_HORIZ_BLANK:
		if (clock - gp->state_start_clock >= GPU_HORIZ_BLANK_TIMING) {
			gp->state_start_clock = 0;
			gp->cur_line++;

			if (gp->cur_line == SCREEN_HEIGHT - 1) {
				gp->mode = GPU_VERT_BLANK;
				// Redraw surface
				SDL_Flip(gp->surface);
			} else {
				gp->mode = GPU_SCAN_OAM;
			}
		}
		break;
	case GPU_VERT_BLANK:
		if (clock - gp->state_start_clock >= GPU_VERT_BLANK_TIMING) {
			gp->state_start_clock = 0;
			gp->cur_line++;

			if (gp->cur_line == SCREEN_HEIGHT + 10) {
				gp->mode = GPU_HORIZ_BLANK;
				gp->cur_line = 0;
			}
		}
		break;
	case GPU_SCAN_OAM:
		if (clock - gp->state_start_clock >= GPU_SCAN_OAM_TIMING) {
			gp->state_start_clock = 0;
			gp->mode = GPU_SCAN_VRAM;
		}
		break;
	case GPU_SCAN_VRAM:
		if (clock - gp->state_start_clock >= GPU_SCAN_VRAM_TIMING) {
			gp->state_start_clock = 0;
			gp->mode = GPU_HORIZ_BLANK;

			// Render one line
			gpu_render(gp);
		}
		break;
	}
}
