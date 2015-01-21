#include <SDL/SDL.h>
#include "log.h"
#include "gpu.h"

gpu* gpu_init(memory *mem) {
	gpu* gp = malloc(sizeof(gpu));
	if (gp == NULL)
		ERROR("Unable to allocate memory for gpu.\n");

	if (SDL_Init(SDL_INIT_VIDEO) == -1)
		ERROR("Unable to load SDL: %s\n", SDL_GetError());

	gp->surface = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, 8, SDL_HWSURFACE | SDL_DOUBLEBUF);
	if (gp->surface == NULL)
		ERROR("Unable to get the SDL surface: %s\n", SDL_GetError());

	// Fill with white
	if (SDL_FillRect(gp->surface, NULL, SDL_MapRGB(gp->surface->format, 0xFF, 0xFF, 0xFF)) == -1)
		ERROR("Unable to fill surface with white: %s\n", SDL_GetError());

	if (SDL_Flip(gp->surface) == -1)
		ERROR("Unable to flip surface at init: %s\n", SDL_GetError());

	gp->vram = calloc(0x2000, sizeof(uint8_t));
	if (gp->vram == NULL)
		ERROR("Unable to allocate memory for gpu.\n");
	memory_set_gpu(mem, gp->vram);

	gp->state_start_clock = 0;
	gp->cur_line = 0;
	gp->scroll_x = 0;
	gp->scroll_y = 0;
	gp->pal = 0;
	gp->tile_set = 0;
	gp->cur_map = 0;
	gp->mode = GPU_HORIZ_BLANK;

	return gp;
}

void gpu_end(gpu *gp) {
	SDL_Quit();
	free(gp->vram);
	free(gp);
}

static uint8_t gpu_get_pixel_color(gpu *gp, uint8_t x, uint8_t y) {
	// Get map offset
	uint16_t map_offset = (gp->cur_map == 0 ? 0x9800 : 0x9C00);
	map_offset -= 0x8000;
	map_offset += (y / 8) * MAP_LINE_WIDTH + x / 8;

	uint8_t tile_offset = gp->vram[map_offset];

	// Get tile
	uint16_t tile_addr = NULL;
	if (gp->tile_set == 0)
		tile_addr = (0x9000 - 0x8000) + ((int8_t)tile_offset) * TILE_HEIGHT * TILE_ENCODED_SIZE;
	else
		tile_addr = (0x8000 - 0x8000) + tile_offset * TILE_HEIGHT * TILE_ENCODED_SIZE;

	// Convert map coordinates to tile coordinate
	uint8_t tile_x = (x % 8);
	uint8_t tile_y = (y % 8);

	tile_addr += tile_y * TILE_ENCODED_SIZE;

	uint8_t tile_content_first = gp->vram[tile_addr];
	uint8_t tile_content_second = gp->vram[tile_addr + 1];

	uint8_t pixel_value = ((tile_content_first << (7 - tile_x)) >> (7 - tile_x)) | ((tile_content_second << (7 - tile_x)) >> (7 - tile_x)) << 1;

	// Convert using current pal
	return (gp->pal & (3 << (pixel_value * 2))) >> (pixel_value * 2);
}

static void gpu_render(gpu *gp) {
	uint8_t x = 0;
	uint8_t y = 0;

	for (y = 0; x < TILE_HEIGHT; y++) {
		for (x = 0; x < SCREEN_WIDTH; x++) {
			// Wrap x
			uint16_t wx = x + gp->scroll_x;
			if (wx > SCREEN_WIDTH)
				wx -= SCREEN_WIDTH;

			// Wrap y
			uint16_t wy = gp->cur_line * y + gp->scroll_y;
			if (wy > SCREEN_HEIGHT)
				wy -= SCREEN_HEIGHT;

			uint8_t pixel_color = gpu_get_pixel_color(gp, wx, wy);

			// Draw pixel
			SDL_LockSurface(gp->surface);
			uint8_t* pixel = gp->surface->pixels + (y * gp->cur_line) * gp->surface->pitch + x;
			switch(pixel_color) {
			case 0:
				*pixel = 0xFF;
				break;
			case 1:
				*pixel = 0xC0;
				break;
			case 2:
				*pixel = 0x60;
				break;
			case 3:
				*pixel = 0x00;
				break;
			}

			SDL_UnlockSurface(gp->surface);
		}
	}
}

// Timing from http://imrannazar.com/GameBoy-Emulation-in-JavaScript:-GPU-Timings
// TODO: simplify all this & only call SDL_Flip when needed
void gpu_process(gpu* gp, uint16_t clock) {
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
