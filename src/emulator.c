#include <gbc_format.h>
#include <stdlib.h>
#include <string.h>

#include "opcodes.h"

// Intialize the emulator (should call sub-systems init)
void emulator_init() {
	opcodes_init();
}

// Execute a gameboy rom through the emulator
void emulator_execute_rom(GB *rom)
{
	/* For now test opcode size */

	int8_t test[0x100] = {
   /*   0,  1,  2,  3,  4,  5,  6,  7,      8,  9,  A, B,  C,  D, E,  F*/
		4, 12,  8,  8,  4,  4,  8,  4,     20,  8,  8, 8,  4,  4, 8,  4,  //0
		4, 12,  8,  8,  4,  4,  8,  4,     12,  8,  8, 8,  4,  4, 8,  4,  //1
		8, 12,  8,  8,  4,  4,  8,  4,      8,  8,  8, 8,  4,  4, 8,  4,  //2
		8, 12,  8,  8, 12, 12, 12,  4,      8,  8,  8, 8,  4,  4, 8,  4,  //3

		4,  4,  4,  4,  4,  4,  8,  4,      4,  4,  4, 4,  4,  4, 8,  4,  //4
		4,  4,  4,  4,  4,  4,  8,  4,      4,  4,  4, 4,  4,  4, 8,  4,  //5
		4,  4,  4,  4,  4,  4,  8,  4,      4,  4,  4, 4,  4,  4, 8,  4,  //6
		8,  8,  8,  8,  8,  8,  4,  8,      4,  4,  4, 4,  4,  4, 8,  4,  //7

		4,  4,  4,  4,  4,  4,  8,  4,      4,  4,  4, 4,  4,  4, 8,  4,  //8
		4,  4,  4,  4,  4,  4,  8,  4,      4,  4,  4, 4,  4,  4, 8,  4,  //9
		4,  4,  4,  4,  4,  4,  8,  4,      4,  4,  4, 4,  4,  4, 8,  4,  //A
		4,  4,  4,  4,  4,  4,  8,  4,      4,  4,  4, 4,  4,  4, 8,  4,  //B

		8, 12, 12, 16, 12, 16,  8, 16,      8, 16, 12, 0, 12, 24, 8, 16,  //C
		8, 12, 12,  4, 12, 16,  8, 16,      8, 16, 12, 4, 12,  4, 8, 16,  //D
		12, 12,  8,  4,  4, 16,  8, 16,     16,  4, 16, 4,  4,  4, 8, 16,  //E
		12, 12,  8,  4,  4, 16,  8, 16,     12,  8, 16, 4,  0,  4, 8, 16   //F
	};

	uint8_t i = 0;
	memory *mem = NULL;
	state st;

	do {
		int8_t res = opcodes_execute(i, &st, mem);

		if (res != (test[i] / 4))
			printf("For 0x%x = %d -- %d %d\n", i, res == (test[i] / 4), res, test[i] / 4);
		i++;
	} while (i != 0);

	// Initiate memory
	// Initiate graphics
	// Initiate inputs
	// Load ROM in memory
	// Initiate machine state
	// Main loop
	//    Parse OpCode
	//    opcodes_execute(opcode, state, memory);
	//    Update PC
	//    Execute other architecture component if needed
}

int main(int argc, char *argv[]) {

	emulator_execute_rom(NULL);

	return 0;
}
