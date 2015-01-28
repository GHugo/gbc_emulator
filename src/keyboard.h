#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__

#include <stdint.h>

typedef struct keyboard {
	struct {
		uint8_t joyp_first;
		uint8_t joyp_second;
		uint8_t active;
	} reg;
} keyboard;

typedef enum {
	KEY_UNKNOWN = -1,
	FIRST_COL   = 0x10,
	KEY_A       = FIRST_COL + (1 << 0),
	KEY_B       = FIRST_COL + (1 << 1),
	KEY_SELECT  = FIRST_COL + (1 << 2),
	KEY_START   = FIRST_COL + (1 << 3),
	SECOND_COL  = 0x20,
	KEY_RIGHT   = SECOND_COL + (1 << 0),
	KEY_LEFT    = SECOND_COL + (1 << 1),
	KEY_UP      = SECOND_COL + (1 << 2),
	KEY_DOWN    = SECOND_COL + (1 << 3),
} keyboard_key;

keyboard* keyboard_init(memory *mem);
void keyboard_end(keyboard *kb);
void keyboard_process(keyboard *kb, uint16_t clk);
void keyboard_pressed(keyboard* kb, keyboard_key key);
void keyboard_released(keyboard* kb, keyboard_key key);
#endif     // __KEYBOARD_H__
