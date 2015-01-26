#include <stdlib.h>
#include <SDL/SDL.h>
#include "memory.h"
#include "keyboard.h"
#include "log.h"

keyboard* keyboard_init(memory* mem) {
	keyboard *kb = malloc(sizeof(keyboard));
	if (kb == NULL)
		ERROR("Unable to allocate memory for keyboard.\n");

	// All keys are up, no column
	kb->reg.joyp = 0xF;
	mem->kb = kb;
	return kb;
}

void keyboard_end(keyboard *kb) {
	free(kb);
}

#define KEYBOARD_SET(v, c, k) ((v) & ~(k))
#define KEYBOARD_CLEAR(v, c, k) ((v) | (k))

void keyboard_pressed(keyboard* kb, keyboard_key key) {
	if (key > FIRST_COL && key < SECOND_COL)
		kb->reg.joyp = KEYBOARD_SET(kb->reg.joyp, FIRST_COL, key - FIRST_COL);
	else if (key > SECOND_COL)
		kb->reg.joyp = KEYBOARD_SET(kb->reg.joyp, SECOND_COL, key - SECOND_COL);
	else
		ERROR("Unknown pressed key %X\n", key);

	DEBUG("Key down %d -- %X\n", key, kb->reg.joyp);
}

void keyboard_released(keyboard* kb, keyboard_key key) {
	if (key > FIRST_COL && key < SECOND_COL)
		kb->reg.joyp = KEYBOARD_CLEAR(kb->reg.joyp, FIRST_COL, key - FIRST_COL);
	else if (key > SECOND_COL)
		kb->reg.joyp = KEYBOARD_CLEAR(kb->reg.joyp, SECOND_COL, key - SECOND_COL);
	else
		ERROR("Unknown released key %X\n", key);
	DEBUG("Key up %d -- %X\n", key, kb->reg.joyp);
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

void keyboard_process(keyboard *kb, uint16_t clk) {
	SDL_Event event;
	// TODO: maybe need multiple PollEvent later
	if (SDL_PollEvent(&event)) {
		switch(event.type)
		{
		case SDL_KEYDOWN:
		{
			keyboard_key key = sdl_to_key(event.key.keysym.sym);
			if (key != KEY_UNKNOWN)
				keyboard_pressed(kb, key);
			break;
		}
		case SDL_KEYUP:
		{
			keyboard_key key = sdl_to_key(event.key.keysym.sym);
			if (key != KEY_UNKNOWN)
				keyboard_released(kb, key);
			break;

		}
		}
	}
}
