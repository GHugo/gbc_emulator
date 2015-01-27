#include <SDL/SDL.h>
#include "log.h"
#include "memory.h"
#include "gpu.h"
#include "interrupts.h"

#define GPU_SET_MODE(gp, mode) (gp)->reg.status = ((gp)->reg.status & 0xFC) | (mode)
#define GPU_GET_MODE(gp) ((gp)->reg.status & 0x3)

gpu* gpu_init(memory *mem) {
	gpu* gp = malloc(sizeof(gpu));
	if (gp == NULL)
		ERROR("Unable to allocate memory for gpu.\n");

	if (SDL_Init(SDL_INIT_VIDEO) == -1)
		ERROR("Unable to load SDL: %s\n", SDL_GetError());

	gp->surface = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, 8, SDL_HWSURFACE);
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

	gp->oam = malloc(sizeof(uint8_t) * 0xA0);
	if (gp->oam == NULL)
		ERROR("Unable to allocate memory for graphics sprites.\n");
	memory_set_gpu(mem, gp);

	gp->state_start_clock = 0;
	gp->reg.control = 0;
	gp->reg.status = 0;
	gp->reg.cur_line = 0;
	gp->reg.scroll_x = 0;
	gp->reg.scroll_y = 0;
	gp->reg.bg_pal = 0;
	gp->reg.sp_pal_0 = 0;
	gp->reg.sp_pal_1 = 0;
	GPU_SET_MODE(gp, GPU_SCAN_VRAM);

	// Check if size is ok for oam data
	if (sizeof(oam_data) != 4 * sizeof(uint8_t))
		ERROR("Size of oam_data (%lu) != %lu\n", sizeof(oam_data), sizeof(uint8_t));

	return gp;
}

void gpu_end(gpu *gp) {
	SDL_Quit();
	free(gp->vram);
	free(gp->oam);
	free(gp);
}

static uint8_t get_tile_pixel_value(gpu *gp, uint16_t tile_addr, uint8_t tile_x, uint8_t tile_y) {
	uint8_t tile_content_first = gp->vram[tile_addr];
	uint8_t tile_content_second = gp->vram[tile_addr + 1];

	uint8_t pixel_first = (tile_content_first & (1 << (7 - tile_x))) >> (7 - tile_x);
	uint8_t pixel_second = (tile_content_second & (1 << (7 - tile_x))) >> (7 - tile_x);
	uint8_t pixel_value = pixel_first | (pixel_second << 1);

	assert(pixel_value >=0 && pixel_value <= 3);
	return pixel_value;
}

static uint8_t gpu_get_bg_pixel_color(gpu *gp, uint8_t x, uint8_t y) {
	assert(x < MAP_TOTAL_WIDTH && y < MAP_TOTAL_HEIGHT);

	// Get map offset
	uint16_t map_offset = ((gp->reg.control & (1 << 3)) == 0 ? 0x9800 : 0x9C00);
	map_offset -= 0x8000;
	map_offset += (y / 8) * MAP_LINE_WIDTH + x / 8;

	uint8_t tile_offset = gp->vram[map_offset];

	// Get tile
	uint16_t tile_addr = NULL;
	if ((gp->reg.control & (1 << 4)) == 0)
		tile_addr = (0x9000 - 0x8000) + ((int8_t)tile_offset) * TILE_HEIGHT * TILE_ENCODED_SIZE;
	else
		tile_addr = (0x8000 - 0x8000) + tile_offset * TILE_HEIGHT * TILE_ENCODED_SIZE;

	// Convert map coordinates to tile coordinate
	uint8_t tile_x = (x % 8);
	uint8_t tile_y = (y % 8);

	tile_addr += tile_y * TILE_ENCODED_SIZE;

	uint8_t pixel_value = get_tile_pixel_value(gp, tile_addr, tile_x, tile_y);

	// Convert using current pal
	return (gp->reg.bg_pal & (3 << (pixel_value * 2))) >> (pixel_value * 2);
}

static uint8_t gpu_get_sprite_pixel_color(gpu *gp, oam_data *obj, uint8_t x, uint8_t y, uint8_t *error) {
	// Reset error before beginning
	*error = 0;

	// Get tile
	uint8_t tile_offset = obj->tile;
	uint16_t tile_addr = tile_offset * TILE_HEIGHT * TILE_ENCODED_SIZE;

	// Correct obj_y & obj_x
	int16_t obj_y = obj->y - 16;
	int16_t obj_x = obj->x - 8;

	// Check if inside the map
	if (obj_x + x < 0 || obj_x + x >= SCREEN_WIDTH) {
		*error = 1;
		return 0;
	}

	// Get tile coordinates
	uint8_t tile_y = (obj->options & (1 << 6)) ? (TILE_HEIGHT - 1) - (y - obj_y) : y - obj_y;
	assert(tile_y >= 0 && tile_y < TILE_HEIGHT);
	uint8_t tile_x = (obj->options & (1 << 5)) ? (TILE_HEIGHT - 1) - x : x;

	uint8_t pixel_value = get_tile_pixel_value(gp, tile_addr, tile_x, tile_y);

	// 00 is transparent
	if (pixel_value == 0) {
		*error = 1;
		return 0;
	}

	// Check if need to draw
	uint8_t bg_value = gpu_get_bg_pixel_color(gp, x, y);
	if (pixel_value && bg_value && ((obj->options & (1 << 7)) == 0)) {
		DEBUG_GPU("Sprite override BG\n");
		*error = 1;
		return 0;
	}

	// Convert using good pal
	uint8_t pal = (obj->options & 0x8) ? gp->reg.sp_pal_1 : gp->reg.sp_pal_0;
	return (pal & (3 << (pixel_value * 2))) >> (pixel_value * 2);
}

static void draw_pixel_on_surface(SDL_Surface *surface, uint8_t x, uint8_t y, uint8_t pixel_color) {
	SDL_LockSurface(surface);
	uint8_t* pixel = surface->pixels + y * surface->w + x;
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

	SDL_UnlockSurface(surface);
}

static void gpu_render(gpu *gp) {
	uint8_t x = 0;

	// Check LCD is on before rendering
	if ((gp->reg.control & 0x80) == 0)  {
		DEBUG_GPU("Rendering off\n");
		return;
	}

	// Render BG
	if (gp->reg.control & 0x1) {
		// Wrap y
		uint16_t wy = gp->reg.cur_line + gp->reg.scroll_y;
		if (wy > MAP_TOTAL_HEIGHT)
			wy -= MAP_TOTAL_HEIGHT;

		for (x = 0; x < SCREEN_WIDTH; x++) {
			// Wrap x
			uint16_t wx = x + gp->reg.scroll_x;
			if (wx > MAP_TOTAL_WIDTH)
				wx -= MAP_TOTAL_WIDTH;

			uint8_t pixel_color = gpu_get_bg_pixel_color(gp, wx, wy);

			// Draw pixel
			draw_pixel_on_surface(gp->surface, x, gp->reg.cur_line, pixel_color);
		}
	}

	// Render sprite
	if (gp->reg.control & 0x2) {
		uint8_t i = 0;
		oam_data* data = (oam_data*)gp->oam;

		for (i = 0; i < SPRITE_COUNT; i++) {
			oam_data* obj = &(data[i]);
			DEBUG_GPU("[%d] Obj = %d %d %d %X\n", i, obj->y, obj->x, obj->tile, obj->options);

			// Correct obj_y & obj_x
			uint8_t obj_y = obj->y - 16;

			// Sprite is on the cur line
			if (obj_y <= gp->reg.cur_line && (obj_y + SPRITE_HEIGHT) > gp->reg.cur_line) {
				uint8_t x = 0;
				for (x = 0; x < SPRITE_WIDTH; x++) {
					uint8_t error = 0;
					uint8_t pixel_color = gpu_get_sprite_pixel_color(gp, obj, x, gp->reg.cur_line, &error);

					// Constraints are handled by gpu_get_sprite_pixel_color
					if (!error)
						draw_pixel_on_surface(gp->surface, x, gp->reg.cur_line, pixel_color);
				}
			}
		}
	}
}

// Timing from http://imrannazar.com/GameBoy-Emulation-in-JavaScript:-GPU-Timings
void gpu_process(gpu* gp, interrupts* ir, uint16_t clock) {
	gp->state_start_clock += clock;

	switch(GPU_GET_MODE(gp)) {
	case GPU_HORIZ_BLANK:
		if (gp->state_start_clock >= GPU_HORIZ_BLANK_TIMING) {
			gp->state_start_clock = 0;
			gp->reg.cur_line++;

			if (gp->reg.cur_line == SCREEN_HEIGHT) {
				GPU_SET_MODE(gp, GPU_VERT_BLANK);
				// Redraw surface
				SDL_Flip(gp->surface);

				// Raise irq
				interrupts_raise(ir, IRQ_VBLANK);
			} else {
				GPU_SET_MODE(gp, GPU_SCAN_OAM);
			}
		}
		break;
	case GPU_VERT_BLANK:
		if (gp->state_start_clock >= GPU_VERT_BLANK_TIMING) {
			gp->state_start_clock = 0;
			gp->reg.cur_line++;

			if (gp->reg.cur_line > SCREEN_HEIGHT + 10) {
				GPU_SET_MODE(gp, GPU_HORIZ_BLANK);
				gp->reg.cur_line = 0;
				gpu_render(gp);
			}
		}
		break;
	case GPU_SCAN_OAM:
		if (gp->state_start_clock >= GPU_SCAN_OAM_TIMING) {
			gp->state_start_clock = 0;
			GPU_SET_MODE(gp, GPU_SCAN_VRAM);
		}
		break;
	case GPU_SCAN_VRAM:
		if (gp->state_start_clock >= GPU_SCAN_VRAM_TIMING) {
			gp->state_start_clock = 0;
			GPU_SET_MODE(gp, GPU_HORIZ_BLANK);

			// Render one line
			if (gp->reg.cur_line < SCREEN_HEIGHT)
				gpu_render(gp);
		}
		break;
	}
}
