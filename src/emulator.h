#ifndef __EMULATOR_H__
#define __EMULATOR_H__

#include <stdint.h>


typedef struct {
	// Registers
	struct {
		uint8_t A, B, C, D, E, H, L; // General purpose
		uint16_t PC, SP; // Program Counter, Stack Pointer
		uint8_t F; // Flags
	} reg;

	// Clock
	struct {
		uint8_t dur;
	} clk;
} state;

typedef enum {
	FLAG_NONE = 0x0,
	FLAG_CARRY = 0x10,
	FLAG_HALF_CARRY = 0x20,
	FLAG_SUBSTRACTION = 0x40,
	FLAG_ZERO = 0x80
} flag_values;

#endif     // __EMULATOR_H__
