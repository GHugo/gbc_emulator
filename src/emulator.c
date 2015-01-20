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
	memory *mem = memory_init(rom);

	// Initiate graphics
	// Initiate inputs
	// Initiate machine state
	state st;
	memset(&st, 0, sizeof(st));
	st.reg.SP = 0xFFFE;

	// Main loop
	while (1) {
		// Update PC
		if (st.reg.PC == 0x100)
			memory_set_bios(mem, 0);

		// Fetch OpCode
		z80_opcode opcode = memory_read_byte(mem, st.reg.PC);
		st.reg.PC++;

		// Decode/Execute opcode
		printf("Executing 0x%X\n", opcode);
		opcodes_execute(opcode, &st, mem);

		// Execute other architecture component if needed
	}

	// Clean stuff
	memory_end(mem);
}

int main(int argc, char *argv[]) {

	if (argc < 2) {
		printf("Usage: %s gbc_file\n", argv[0]);
		return 0;
	}

	// Load & check GB
	GB *rom = gbc_open(argv[1]);
	gbc_read_header(rom);
	gbc_check_header(rom);

	// Launch emulator
	emulator_execute_rom(rom);

	// Clean stuff
	gbc_close(rom);

	return 0;
}
