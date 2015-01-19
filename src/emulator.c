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
	return 0;
}
