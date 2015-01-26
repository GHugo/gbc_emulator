#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__

#include <stdint.h>

typedef struct keyboard {
	struct {
		uint8_t joyp;
	} reg;
} keyboard;

typedef enum {
	KEY_UNKNOWN = -1,
	FIRST_COL = 0x10,
	KEY_A,
	KEY_B,
	KEY_SELECT,
	KEY_START,
	SECOND_COL = 0x20,
	KEY_RIGHT,
	KEY_LEFT,
	KEY_UP,
	KEY_DOWN,
} keyboard_key;

keyboard* keyboard_init(memory *mem);
void keyboard_end(keyboard *kb);
void keyboard_process(keyboard *kb, uint16_t clk);
void keyboard_pressed(keyboard* kb, keyboard_key key);
void keyboard_released(keyboard* kb, keyboard_key key);
#endif     // __KEYBOARD_H__
