#include <stdlib.h>
#include <SDL/SDL.h>
#include "memory.h"
#include "keyboard.h"
#include "interrupts.h"
#include "log.h"

keyboard* keyboard_init(memory* mem) {
	keyboard *kb = malloc(sizeof(keyboard));
	if (kb == NULL)
		ERROR("Unable to allocate memory for keyboard.\n");

	// All keys are up, no column
	kb->reg.joyp_first = FIRST_COL | 0xF;
	kb->reg.joyp_second = SECOND_COL | 0xF;
	kb->reg.active = 0x0;
	mem->kb = kb;
	return kb;
}

void keyboard_end(keyboard *kb) {
	free(kb);
}

void keyboard_pressed(keyboard* kb, keyboard_key key, interrupts *ir) {
	if (key & FIRST_COL)
		kb->reg.joyp_first = kb->reg.joyp_first & ~(key - FIRST_COL);
	else if (key & SECOND_COL)
		kb->reg.joyp_second = kb->reg.joyp_second & ~(key - SECOND_COL);
	else
		ERROR("Unknown pressed key %X\n", key);

	// Handle interrupts
	if (key & kb->reg.active)
		interrupts_raise(ir, IRQ_JOYPAD);

	DEBUG_KEYBOARD("Key down %d -- %X - %X\n", key, kb->reg.joyp_first, kb->reg.joyp_second);
}

void keyboard_released(keyboard* kb, keyboard_key key, interrupts *ir) {
	if (key & FIRST_COL)
		kb->reg.joyp_first = kb->reg.joyp_first | (key - FIRST_COL);
	else if (key & SECOND_COL)
		kb->reg.joyp_second = kb->reg.joyp_second | (key - SECOND_COL);
	else
		ERROR("Unknown released key %X\n", key);

	DEBUG_KEYBOARD("Key up %d -- %X - %X\n", key, kb->reg.joyp_first, kb->reg.joyp_second);
}

static keyboard_key sdl_to_key(SDLKey key) {
	switch(key) {
	case SDLK_a:
		return KEY_A;
	case SDLK_b:
		return KEY_B;
	case SDLK_l:
		return KEY_SELECT;
	case SDLK_s:
		return KEY_START;
	case SDLK_RIGHT:
		return KEY_RIGHT;
	case SDLK_LEFT:
		return KEY_LEFT;
	case SDLK_UP:
		return KEY_UP;
	case SDLK_DOWN:
		return KEY_DOWN;
	default:
		return KEY_UNKNOWN;
	}
}

void keyboard_process(keyboard *kb, interrupts *ir, uint16_t clk) {
	SDL_Event event;
	// TODO: maybe need multiple PollEvent later
	while (SDL_PollEvent(&event)) {
		switch(event.type)
		{
		case SDL_KEYDOWN:
		{
			keyboard_key key = sdl_to_key(event.key.keysym.sym);
			if (key != KEY_UNKNOWN)
				keyboard_pressed(kb, key, ir);
			break;
		}
		case SDL_KEYUP:
		{
			keyboard_key key = sdl_to_key(event.key.keysym.sym);
			if (key != KEY_UNKNOWN)
				keyboard_released(kb, key, ir);
			break;

		}
		}
	}
}

void keyboard_wait_key(keyboard *kb, interrupts *ir) {
	SDL_Event event;
	while (SDL_WaitEvent(&event)) {
		switch(event.type)
		{
		case SDL_KEYDOWN:
		{
			keyboard_key key = sdl_to_key(event.key.keysym.sym);
			if (key != KEY_UNKNOWN)
				keyboard_pressed(kb, key, ir);
			return;
		}
		case SDL_KEYUP:
		{
			keyboard_key key = sdl_to_key(event.key.keysym.sym);
			if (key != KEY_UNKNOWN)
				keyboard_released(kb, key, ir);
			return;

		}
		}
	}
}
