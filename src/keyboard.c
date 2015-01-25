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

#define KEYBOARD_SET(v, c, k) (((v) | (c)) & ~(k))
#define KEYBOARD_CLEAR(v, c, k) (((v) & ~(c)) | (k))

void keyboard_pressed(keyboard* kb, keyboard_key key) {
	if (key > FIRST_COL && key < SECOND_COL)
		kb->reg.joyp = KEYBOARD_SET(kb->reg.joyp, FIRST_COL, key - FIRST_COL);
	else if (key > SECOND_COL)
		kb->reg.joyp = KEYBOARD_SET(kb->reg.joyp, SECOND_COL, key - SECOND_COL);
	else
		ERROR("Unknown pressed key %X\n", key);
}

void keyboard_released(keyboard* kb, keyboard_key key) {
	if (key > FIRST_COL && key < SECOND_COL)
		kb->reg.joyp = KEYBOARD_CLEAR(kb->reg.joyp, FIRST_COL, key - FIRST_COL);
	else if (key > SECOND_COL)
		kb->reg.joyp = KEYBOARD_CLEAR(kb->reg.joyp, SECOND_COL, key - SECOND_COL);
	else
		ERROR("Unknown released key %X\n", key);
}

void keyboard_process(keyboard *kb, uint16_t clk) {
	SDL_Event event;
	// TODO: maybe need multiple PollEvent later
	while (SDL_PollEvent(&event)) {
		switch(event.type)
		{
		case SDL_KEYDOWN:
			keyboard[event.key.keysym.sym] = false;
			break;
		case SDL_KEYUP:
			keyboard[event.key.keysym.sym] = true;
			break;
		}
	}
}
