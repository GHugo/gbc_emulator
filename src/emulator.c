#include <gbc_format.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "log.h"
#include "opcodes.h"
#include "gpu.h"

// Intialize the emulator (should call sub-systems init)
void emulator_init() {
	opcodes_init();
}

#include <stdlib.h>
#include <stdio.h>
#include <SDL/SDL.h>

void pause_sdl()
{
	int cont = 1;
	SDL_Event event;

	while (cont)
	{
		SDL_WaitEvent(&event);
		switch(event.type)
		{
		case SDL_QUIT:
			exit(0);
		}
	}
}

void dump_states(state *st) {
	DEBUG("A = %X\n", st->reg.A);
	DEBUG("B = %X\n", st->reg.B);
	DEBUG("C = %X\n", st->reg.C);
	DEBUG("D = %X\n", st->reg.D);
	DEBUG("E = %X\n", st->reg.E);
	DEBUG("H = %X\n", st->reg.H);
	DEBUG("L = %X\n", st->reg.L);
	DEBUG("HL = %X\n", (st->reg.H << 8) + st->reg.L);
	DEBUG("SP = %X\n", st->reg.SP);
	DEBUG("PC = %X\n", st->reg.PC);
	DEBUG("F = %X\n", st->reg.F);
}

// Execute a gameboy rom through the emulator
void emulator_execute_rom(GB *rom)
{
	// Initiate memory
	memory *mem = memory_init(rom);

	// Initiate graphics
	gpu* gp = gpu_init(mem);

	// Initiate inputs
	// Initiate machine state
	state st;
	memset(&st, 0, sizeof(st));
	st.reg.SP = 0xFFFE;
	st.reg.A = 0x01;
	st.reg.F = 0xB0;
	st.reg.C = 0x13;
	st.reg.E = 0xD8;
	st.reg.H = 0x01;
	st.reg.L = 0x4D;
	st.clk = 0;

	// Main loop
	uint16_t last_pause = 0;
	uint16_t bp = 0xe0;
	uint16_t bp_seen = 0;
	while (1) {
		// Update PC
		if (st.reg.PC == 0x100) {
			ERROR("Not in Bios\n");
			memory_set_bios(mem, 0);
		}

		// Fetch OpCode
		z80_opcode opcode = memory_read_byte(mem, st.reg.PC);
		st.reg.PC++;

		// Decode/Execute opcode
		DEBUG("======================\n");
		DEBUG("Executing 0x%X\n", opcode);

		int8_t clk = opcodes_execute(opcode, &st, mem);
		if (clk < 0)
			ERROR("Unknown operation!\n");

		dump_states(&st);

		st.clk += clk;
		last_pause += clk;

		// Execute other architecture component if needed
		gpu_process(gp, clk);

		if (st.reg.PC == bp)
			bp_seen = 1;

		// Sleep each frame
		if (bp_seen/*last_pause >= 17556*/) {
			//DEBUG("pause...\n");
			usleep(1);
			getchar();
			last_pause = 0;
		}
	}

	// Clean stuff
	memory_end(mem);
	gpu_end(gp);
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
