#include <stdlib.h>
#include "opcodes.h"
#include "log.h"

// Init opcodes if needed
void opcodes_init() {
}

// All opcode functions below.

// An opcode function. Take the current machine state and the memory and return
// the number of cycles taken by the instruction.


// Decide which register is concerned.
// Return NULL if (HL) register
static uint8_t* resolve_register(uint8_t z, state *st) {
	switch (z) {
	case 0:
		return &(st->reg.B);
	case 1:
		return &(st->reg.C);
	case 2:
		return &(st->reg.D);
	case 3:
		return &(st->reg.E);
	case 4:
		return &(st->reg.H);
	case 5:
		return &(st->reg.L);
	case 6:
		return NULL;
	case 7:
		return &(st->reg.A);
	}

	ERROR("Resolve register for unknown z=%d\n", z);
	return NULL;
}

#ifndef NDEBUG_OPCODES
static const char* resolve_register_name(uint8_t z) {
	switch (z) {
	case 0:
		return "B";
	case 1:
		return "C";
	case 2:
		return "D";
	case 3:
		return "E";
	case 4:
		return "H";
	case 5:
		return "L";
	case 6:
		return "(HL)";
	case 7:
		return "A";
	}
	return "?";
}
#endif

// Extended instructions (rotation related)
static int8_t handle_roll(uint8_t y, uint8_t z, state *st, memory* mem) {
	uint8_t* reg = resolve_register(z, st);
	uint8_t ci = 0;
	uint8_t co = 0;

	// Get data from memory for (HL)
	uint8_t is_hl = reg == NULL;
	uint8_t hl_value = 0;
	if (is_hl) {
		hl_value = memory_read_byte(mem, (st->reg.H << 8) + st->reg.L);
		reg = &hl_value;
	}

	switch(y) {
		// RLC
	case 0:
		DEBUG_OPCODES("RLC %s\n", resolve_register_name(z));
		ci = *reg & 0x80 ? 1 : 0;
		co = *reg & 0x80 ? FLAG_CARRY : 0;
		*reg = (*reg << 1);

		break;

		// RRC
	case 1:
		DEBUG_OPCODES("RRC %s\n", resolve_register_name(z));
		ci = *reg & 0x1 ? 0x80 : 0;
		co = *reg & 0x1 ? FLAG_CARRY : 0;
		*reg = (*reg >> 1);
		break;

		// RL
	case 2:
		DEBUG_OPCODES("RL %s\n", resolve_register_name(z));
		ci = st->reg.F & FLAG_CARRY ? 1 : 0;
		co = *reg & 0x80 ? FLAG_CARRY : 0;
		*reg = (*reg << 1);
		break;

		// RR
	case 3:
		DEBUG_OPCODES("RR %s\n", resolve_register_name(z));
		ci = st->reg.F & FLAG_CARRY ? 0x80 : 0;
		co = *reg & 0x1 ? FLAG_CARRY : 0;
		*reg = (*reg >> 1);
		break;

		// SLA
	case 4:
		DEBUG_OPCODES("SLA %s\n", resolve_register_name(z));
		ci = 0;
		co = *reg & 0x80 ? FLAG_CARRY : 0;
		*reg = (*reg << 1);
		break;

		// SRA
	case 5:
		DEBUG_OPCODES("SRA %s\n", resolve_register_name(z));
		ci = *reg & 0x80;
		co = *reg & 0x1 ? FLAG_CARRY : 0;
		*reg = (*reg >> 1);
		break;

		// Special gameboy case SWAP
	case 6:
		DEBUG_OPCODES("SWAP %s\n", resolve_register_name(z));
		ci = 0;
		co = 0;
		*reg = ((*reg&0x0F) << 4) | ((*reg & 0xF0) >> 4);
		st->reg.F &= ~FLAG_CARRY;
		break;

		// SRL
	case 7:
		DEBUG_OPCODES("SRL %s\n", resolve_register_name(z));
		ci = 0;
		co = *reg & 0x1 ? FLAG_CARRY : 0;
		*reg = (*reg >> 1);
		break;
	}

    // General operations
	*reg += ci;
	st->reg.F = *reg ? 0 : FLAG_ZERO;
	st->reg.F |= co;

	st->reg.F &= ~FLAG_HALF_CARRY;
	st->reg.F &= ~FLAG_SUBSTRACTION;

	// Write result to memory
	if (is_hl) {
		memory_write_byte(mem, (st->reg.H << 8) + st->reg.L, *reg);
		return 4;
	} else {
		return 2;
	}
}

// Check if a bit is set -- BIT y, r[z]
static int8_t handle_bit(uint8_t y, uint8_t z, state *st, memory* mem) {
	uint8_t* reg = resolve_register(z, st);

	DEBUG_OPCODES("BIT %d, %s\n", y, resolve_register_name(z));

	// Get data from memory for (HL)
	uint8_t is_hl = reg == NULL;
	uint8_t hl_value = 0;
	if (is_hl) {
		hl_value = memory_read_byte(mem, (st->reg.H << 8) + st->reg.L);
		reg = &hl_value;
	}

	st->reg.F &= ~FLAG_SUBSTRACTION;
	st->reg.F |= FLAG_HALF_CARRY;
	st->reg.F &= ~FLAG_ZERO;
	st->reg.F |= *reg & (1 << y) ? 0 : FLAG_ZERO;

	if (is_hl)
		return 3;
	else
		return 2;
}

// Set to 0 a specific bit -- RES y, r[z]
static int8_t handle_res(uint8_t y, uint8_t z, state *st, memory* mem) {
	uint8_t* reg = resolve_register(z, st);

	DEBUG_OPCODES("RES %d, %s\n", y, resolve_register_name(z));

	// Get data from memory for (HL)
	uint8_t is_hl = reg == NULL;
	uint8_t hl_value = 0;
	if (is_hl) {
		hl_value = memory_read_byte(mem, (st->reg.H << 8) + st->reg.L);
		reg = &hl_value;
	}

	*reg &= ~(1 << y);

	if (is_hl) {
		memory_write_byte(mem, (st->reg.H << 8) + st->reg.L, *reg);
		return 4;
	} else {
		return 2;
	}
}

// Set to 1 a specific bit -- SET y, r[z]
static int8_t handle_set(uint8_t y, uint8_t z, state *st, memory* mem) {
	uint8_t* reg = resolve_register(z, st);

	DEBUG_OPCODES("SET %d, %s\n", y, resolve_register_name(z));

	// Get data from memory for (HL)
	uint8_t is_hl = reg == NULL;
	uint8_t hl_value = 0;
	if (is_hl) {
		hl_value = memory_read_byte(mem, (st->reg.H << 8) + st->reg.L);
		reg = &hl_value;
	}

	*reg |= 1 << y;

	if (is_hl) {
		memory_write_byte(mem, (st->reg.H << 8) + st->reg.L, *reg);
		return 4;
	} else {
		return 2;
	}
}

#ifndef NDEBUG_OPCODES
static const char* resolve_cc(uint8_t y) {
	switch(y) {
	case 0:
		return "NZ";
	case 1:
		return "Z";
	case 2:
		return "NC";
	case 3:
		return "C";
	default:
		return "?";
	}
}
#endif

// Relative jumps and assorted ops
static int8_t handle_no_extra_x_0_z_0(uint8_t y, uint8_t z, uint8_t p, uint8_t q, state* st, memory* mem) {
	switch (y) {
		// NOP
	case 0:
		DEBUG_OPCODES("NOP\n");
		return 1;

		// LD (nn), SP
	case 1:
	{
		uint16_t nn = memory_read_word(mem, st->reg.PC);
		st->reg.PC += 2;
		DEBUG_OPCODES("LD (nn=%X), SP\n", nn);

		memory_write_word(mem, nn, st->reg.SP);

		return 5;
	}
		// STOP
	case 2:
	{
		DEBUG_OPCODES("STOP\n");

		WARN("STOP mode asked, need to be handled.\n");
		return 1;
	}
		// JR d
	case 3:
	{
		int8_t nn = memory_read_byte(mem, st->reg.PC);
		st->reg.PC++;

		DEBUG_OPCODES("JR %d\n", nn);

		st->reg.PC += nn;
		return 3;
	}
		// JR cc[y-4], d
	case 4:
	case 5:
	case 6:
	case 7:
	{
		int8_t nn = memory_read_byte(mem, st->reg.PC);
		st->reg.PC++;
		uint8_t do_jump = 0;

		DEBUG_OPCODES("JR %s, %d\n", resolve_cc(y-4), nn);

		// JR NZ, d
		if (y == 4)
			do_jump = (st->reg.F & FLAG_ZERO) == 0;
		// JR Z, d
		else if (y == 5)
			do_jump = (st->reg.F & FLAG_ZERO) != 0;
		// JR NC, d
		else if (y == 6)
			do_jump = (st->reg.F & FLAG_CARRY) == 0;
		// JR C, d
		else if (y == 7)
			do_jump = (st->reg.F & FLAG_CARRY) != 0;

		if (do_jump) {
			st->reg.PC += nn;
			return 3;
		} else {
			return 2;
		}
	}
	}

	return -1;
}

// Select register using register pairs table
static void resolve_register_pairs(state *st, uint8_t p, uint8_t **first, uint8_t **second) {
	switch(p) {
	case 0:
		*first = &(st->reg.B);
		*second = &(st->reg.C);
		break;
	case 1:
		*first = &(st->reg.D);
		*second = &(st->reg.E);
		break;
	case 2:
		*first = &(st->reg.H);
		*second = &(st->reg.L);
		break;
	case 3:
		// Trick for SP to fit, suppose little-endianness
		*second = (uint8_t*)(&(st->reg.SP));
		*first = (uint8_t*)(&(st->reg.SP)) + sizeof(uint8_t);
		break;
	default:
		ERROR("Unknown register pairs with p = %d\n", p);
	}
}

#ifndef NDEBUG_OPCODES
static const char* resolve_register_pairs_name(uint8_t p) {
	switch(p) {
	case 0:
		return "BC";
	case 1:
		return "DE";
	case 2:
		return "HL";
	case 3:
		return "SP";
	default:
		return "??";
	}
}
#endif

// Select register using register pairs table version 2
static void resolve_register_pairs_v2(state *st, uint8_t p, uint8_t **first, uint8_t **second) {
	switch(p) {
	case 0:
		*first = &(st->reg.B);
		*second = &(st->reg.C);
		break;
	case 1:
		*first = &(st->reg.D);
		*second = &(st->reg.E);
		break;
	case 2:
		*first = &(st->reg.H);
		*second = &(st->reg.L);
		break;
	case 3:
		*first = &(st->reg.A);
		*second = &(st->reg.F);
		break;
	default:
		ERROR("Unknown register pairs v2 with p = %d\n", p);
	}
}

#ifndef NDEBUG_OPCODES
static const char* resolve_register_pairs_v2_name(uint8_t p) {
	switch(p) {
	case 0:
		return "BC";
	case 1:
		return "DE";
	case 2:
		return "HL";
	case 3:
		return "AF";
	default:
		return "??";
	}
}
#endif

// 16-bit load immediate/add
static int8_t handle_no_extra_x_0_z_1(uint8_t y, uint8_t z, uint8_t p, uint8_t q, state* st, memory* mem) {
	uint8_t *first = NULL;
	uint8_t *second = NULL;
	resolve_register_pairs(st, p, &first, &second);

	// LD rp[p], nn
	if (q == 0) {
		*second = memory_read_byte(mem, st->reg.PC);
		st->reg.PC++;
		*first = memory_read_byte(mem, st->reg.PC);
		st->reg.PC++;

		DEBUG_OPCODES("LD %s, %X\n", resolve_register_pairs_name(p), (*second << 8) + *first);

		return 3;
    // ADD HL, rp[p]
	} else {
		DEBUG_OPCODES("ADD HL, %s\n", resolve_register_pairs_name(p));

		uint16_t hl = (st->reg.H << 8) + st->reg.L;
		uint16_t pr = (*first << 8) + *second;
		uint32_t res = hl + pr;

		if ((hl & 0xFFF) > (res & 0xFFF))
			st->reg.F |= FLAG_HALF_CARRY;
		else
			st->reg.F &= ~FLAG_HALF_CARRY;

		if (res > 0xFFFF)
			st->reg.F |= FLAG_CARRY;
		else
			st->reg.F &= ~FLAG_CARRY;

		st->reg.F &= ~FLAG_SUBSTRACTION;

		st->reg.H = (res >> 8) & 0xFF;
		st->reg.L = (res & 0xFF);
		return 2;
	}
}

// Indirect loading
static int8_t handle_no_extra_x_0_z_2(uint8_t y, uint8_t z, uint8_t p, uint8_t q, state* st, memory* mem) {
	switch((q << 2) + p) {
		// LD (BC), A
	case 0:
		DEBUG_OPCODES("ADD (BC), A\n");

		memory_write_byte(mem, (st->reg.B << 8) + st->reg.C, st->reg.A);
		return 2;
		// LD (DE), A
	case 1:
		DEBUG_OPCODES("LD (DE), A\n");

		memory_write_byte(mem, (st->reg.D << 8) + st->reg.E, st->reg.A);
		return 2;
		// LDI (HL), A
	case 2:
	{
		DEBUG_OPCODES("LDI (HL), A\n");

		uint16_t hl = (st->reg.H << 8) + st->reg.L;
		memory_write_byte(mem, hl, st->reg.A);
		hl++;
		st->reg.H = hl >> 8;
		st->reg.L = hl;

		return 2;
	}
		// LDD (HL), A
	case 3:
	{
		DEBUG_OPCODES("LDD (HL), A\n");
		uint16_t hl = (st->reg.H << 8) + st->reg.L;
		memory_write_byte(mem, hl, st->reg.A);
		hl--;
		st->reg.H = hl >> 8;
		st->reg.L = hl;

		return 2;
	}
		// LD A, (BC)
	case 4:
		DEBUG_OPCODES("LD A, (BC)\n");

		st->reg.A = memory_read_byte(mem, (st->reg.B << 8) + st->reg.C);
		return 2;
		// LD A, (DE)
	case 5:
		DEBUG_OPCODES("LD A, (DE)\n");

		st->reg.A = memory_read_byte(mem, (st->reg.D << 8) + st->reg.E);
		return 2;
		// LDI A, (HL)
	case 6:
	{
		DEBUG_OPCODES("LDI A, (HL)\n");

		uint16_t hl = (st->reg.H << 8) + st->reg.L;
		st->reg.A = memory_read_byte(mem, hl);
		hl++;
		st->reg.H = hl >> 8;
		st->reg.L = hl;

		return 2;
	}
		// LDD A, (HL)
	case 7:
	{
		DEBUG_OPCODES("LDD A, (HL)\n");

		uint16_t hl = (st->reg.H << 8) + st->reg.L;
		st->reg.A = memory_read_byte(mem, hl);
		hl--;
		st->reg.H = hl >> 8;
		st->reg.L = hl;

		return 2;
	}
	}

	return -1;
}

// 16-bit INC/DEC
static int8_t handle_no_extra_x_0_z_3(uint8_t y, uint8_t z, uint8_t p, uint8_t q, state* st, memory* mem) {
	uint8_t *first = NULL;
	uint8_t *second = NULL;
	resolve_register_pairs(st, p, &first, &second);

	uint16_t total = (*first << 8) + *second;


	// INC rp[p]
	if (q == 0) {
		DEBUG_OPCODES("INC %s\n", resolve_register_pairs_name(p));
		total++;
	}
	// DEC rp[p]
	else {
		total--;
		DEBUG_OPCODES("INC %s\n", resolve_register_pairs_name(p));
	}

	*first = total >> 8;
	*second = total;

	return 2;
}

// 8-bit INC
static int8_t handle_no_extra_x_0_z_4(uint8_t y, uint8_t z, uint8_t p, uint8_t q, state* st, memory* mem) {
	uint8_t *reg = resolve_register(y, st);

	DEBUG_OPCODES("INC %s\n", resolve_register_name(y));

	// Get data from memory for (HL)
	uint8_t is_hl = reg == NULL;
	uint8_t hl_value = 0;
	if (is_hl) {
		hl_value = memory_read_byte(mem, (st->reg.H << 8) + st->reg.L);
		reg = &hl_value;
	}

	*reg += 1;

	// Handle flags
	if (*reg == 0)
		st->reg.F |= FLAG_ZERO;
	else
		st->reg.F &= ~FLAG_ZERO;

	if ((*reg & 0xF) == 0)
		st->reg.F |= FLAG_HALF_CARRY;
	else
		st->reg.F &= ~FLAG_HALF_CARRY;

	st->reg.F &= ~FLAG_SUBSTRACTION;

	if (is_hl) {
		memory_write_byte(mem, (st->reg.H << 8) + st->reg.L, *reg);
		return 3;
	} else {
		return 1;
	}

	return 1;
}

// 8-bit DEC
static int8_t handle_no_extra_x_0_z_5(uint8_t y, uint8_t z, uint8_t p, uint8_t q, state* st, memory* mem) {
	uint8_t *reg = resolve_register(y, st);

	DEBUG_OPCODES("DEC %s\n", resolve_register_name(y));

	// Get data from memory for (HL)
	uint8_t is_hl = reg == NULL;
	uint8_t hl_value = 0;
	if (is_hl) {
		hl_value = memory_read_byte(mem, (st->reg.H << 8) + st->reg.L);
		reg = &hl_value;
	}

	*reg -= 1;

	// Handle flags
	if (*reg == 0)
		st->reg.F |= FLAG_ZERO;
	else
		st->reg.F &= ~FLAG_ZERO;

	if ((*reg & 0xF) == 0xF)
		st->reg.F |= FLAG_HALF_CARRY;
	else
		st->reg.F &= ~FLAG_HALF_CARRY;

	st->reg.F |= FLAG_SUBSTRACTION;

	if (is_hl) {
		memory_write_byte(mem, (st->reg.H << 8) + st->reg.L, *reg);
		return 3;
	} else {
		return 1;
	}

	return 1;
}

// 8-bit load immediate -- LD r[y], nn
static int8_t handle_no_extra_x_0_z_6(uint8_t y, uint8_t z, uint8_t p, uint8_t q, state* st, memory* mem) {
	uint8_t *reg = resolve_register(y, st);

	// Get data from memory for (HL)
	uint8_t is_hl = reg == NULL;
	uint8_t hl_value = 0;
	if (is_hl) {
		hl_value = memory_read_byte(mem, (st->reg.H << 8) + st->reg.L);
		reg = &hl_value;
	}

	uint8_t im = memory_read_byte(mem, st->reg.PC);
	st->reg.PC++;

	DEBUG_OPCODES("LD %s, %X\n", resolve_register_name(y), im);

	*reg = im;

	if (is_hl) {
		memory_write_byte(mem, (st->reg.H << 8) + st->reg.L, *reg);
		return 3;
	}

	return 2;
}

// Assorted operations on accumulator/flags
static int8_t handle_no_extra_x_0_z_7(uint8_t y, uint8_t z, uint8_t p, uint8_t q, state* st, memory* mem) {
	switch (y) {
		// RLCA
	case 0:
	{
		DEBUG_OPCODES("RLCA\n");

		uint8_t new = st->reg.A << 1 | st->reg.A >> 7;

		st->reg.F = FLAG_NONE;

		if (st->reg.A > 0x7F)
			st->reg.F |= FLAG_CARRY;

		st->reg.A = new;
		break;
	}
		// RRCA
	case 1:
	{
		DEBUG_OPCODES("RRCA\n");

		uint8_t new = st->reg.A >> 1 | st->reg.A << 7;

		st->reg.F = FLAG_NONE;

		if (new > 0x7F)
			st->reg.F |= FLAG_CARRY;

		st->reg.A = new;
		break;
	}
		// RLA
	case 2:
	{
		DEBUG_OPCODES("RLA\n");

		uint8_t new = st->reg.A << 1;

		if (st->reg.F & FLAG_CARRY)
			new += 1;

		st->reg.F = FLAG_NONE;

		if (st->reg.A > 0x7F)
			st->reg.F |= FLAG_CARRY;

		st->reg.A = new;
		break;
	}
		// RRA
	case 3:
	{
		DEBUG_OPCODES("RRA\n");

		uint8_t new = st->reg.A >> 1;

		if (st->reg.F & FLAG_CARRY)
			new |= 0x80;

		st->reg.F = FLAG_NONE;

		if (st->reg.A & 1)
			st->reg.F |= FLAG_CARRY;

		st->reg.A = new;
		break;
	}
		// DAA
		// This one from https://raw.githubusercontent.com/grantgalitz/GameBoy-Online/master/js/GameBoyCore.js
	case 4:
	{
		DEBUG_OPCODES("DAA\n");

		if ((st->reg.F & FLAG_SUBSTRACTION) == 0) {
			if (((st->reg.F & FLAG_CARRY) != 0) || st->reg.A > 0x99) {
				st->reg.A += 0x60;
				st->reg.F |= FLAG_CARRY;
			}
			if (((st->reg.F & FLAG_HALF_CARRY) != 0) || st->reg.A > 0x9) {
				st->reg.A += 0x06;
				st->reg.F &= ~FLAG_HALF_CARRY;
			}
		}
		else if ((st->reg.F & FLAG_CARRY) != 0 && (st->reg.F & FLAG_HALF_CARRY) != 0) {
			st->reg.A += 0x9A;
			st->reg.F &= ~FLAG_HALF_CARRY;
		}
		else if ((st->reg.F & FLAG_CARRY) != 0) {
			st->reg.A += 0xA0;
		}
		else if ((st->reg.F & FLAG_HALF_CARRY) != 0) {
			st->reg.A += 0xFA;
			st->reg.F &= ~FLAG_HALF_CARRY;
		}

		if (st->reg.A > 0)
			st->reg.F &= ~FLAG_ZERO;
		else
			st->reg.F |= FLAG_ZERO;

		break;
	}
		// CPL
	case 5:
		DEBUG_OPCODES("CPL\n");

		st->reg.A = ~(st->reg.A);
		st->reg.F |= FLAG_SUBSTRACTION | FLAG_CARRY;

		break;
		// SCF
	case 6:
		DEBUG_OPCODES("SCF\n");

		st->reg.F |= FLAG_CARRY;
		st->reg.F &= ~FLAG_SUBSTRACTION;
		st->reg.F &= ~FLAG_HALF_CARRY;
		break;

		// CCF
	case 7:
		DEBUG_OPCODES("CCF\n");

		st->reg.F = st->reg.F ^ FLAG_CARRY;
		st->reg.F &= ~FLAG_SUBSTRACTION;
		st->reg.F &= ~FLAG_HALF_CARRY;
		break;
	}

	return 1;
}

static int8_t handle_no_extra_x_0(uint8_t y, uint8_t z, uint8_t p, uint8_t q, state* st, memory* mem) {
	switch (z) {
	case 0:
		return handle_no_extra_x_0_z_0(y, z, p, q, st, mem);
	case 1:
		return handle_no_extra_x_0_z_1(y, z, p, q, st, mem);
	case 2:
		return handle_no_extra_x_0_z_2(y, z, p, q, st, mem);
	case 3:
		return handle_no_extra_x_0_z_3(y, z, p, q, st, mem);
	case 4:
		return handle_no_extra_x_0_z_4(y, z, p, q, st, mem);
	case 5:
		return handle_no_extra_x_0_z_5(y, z, p, q, st, mem);
	case 6:
		return handle_no_extra_x_0_z_6(y, z, p, q, st, mem);
	case 7:
		return handle_no_extra_x_0_z_7(y, z, p, q, st, mem);
	}

	return -1;
}

// Load reg_src into reg_dst -- LD r[y], r[z]
static int8_t handle_no_extra_x_1(uint8_t y, uint8_t z, uint8_t p, uint8_t q, state* st, memory* mem) {
	uint8_t* reg_dst = resolve_register(y, st);
	uint8_t* reg_src = resolve_register(z, st);

	// Get data from memory for (HL)
	uint8_t is_hl_dst = reg_dst == NULL;
	uint8_t is_hl_src = reg_src == NULL;
	uint8_t hl_value = 0;

	// Special case of HALT instruction -- replace LD (HL), (HL)
	if (is_hl_dst && is_hl_src) {
		DEBUG_OPCODES("HALT\n");

		WARN("HALT instruction, not handled yet.\n");
		return 1;
	}

	DEBUG_OPCODES("LD %s, %s\n", resolve_register_name(y), resolve_register_name(z));

	if (is_hl_src) {
		hl_value = memory_read_byte(mem, (st->reg.H << 8) + st->reg.L);
		reg_src = &hl_value;
	}

	if (is_hl_dst) {
		reg_dst = &hl_value;
	}

	*reg_dst = *reg_src;

	if (is_hl_dst)
		memory_write_byte(mem, (st->reg.H << 8) + st->reg.L, *reg_dst);

	if (is_hl_src || is_hl_dst)
		return 2;
	else
		return 1;
}

// Registers arithmetic
// TODO: refactor
static int8_t handle_no_extra_x_2(uint8_t y, uint8_t z, uint8_t p, uint8_t q, state* st, memory* mem) {
	uint8_t* reg = resolve_register(z, st);

	// Get data from memory for (HL)
	uint8_t is_hl = reg == NULL;
	uint8_t hl_value = 0;
	if (is_hl) {
		hl_value = memory_read_byte(mem, (st->reg.H << 8) + st->reg.L);
		reg = &hl_value;
	}

	switch(y) {
		// ADD A,
	case 0:
	{
		DEBUG_OPCODES("ADD A, %s\n", resolve_register_name(z));

		uint16_t bound = st->reg.A + *reg;

		st->reg.F = FLAG_NONE;

		if ((bound & 0xF) < (st->reg.A & 0xF))
			st->reg.F |= FLAG_HALF_CARRY;

		if (bound > 0xFF)
			st->reg.F |= FLAG_CARRY;

		st->reg.A = (uint8_t)bound;

		if (st->reg.A == 0)
			st->reg.F |= FLAG_ZERO;

		break;
	}
		// ADC A,
	case 1:
	{
		DEBUG_OPCODES("ADC A, %s\n", resolve_register_name(z));

		uint16_t bound = st->reg.A + *reg + (st->reg.F & FLAG_CARRY ? 1 : 0);

		st->reg.F = FLAG_NONE;

		if (bound > 0xF)
			st->reg.F |= FLAG_HALF_CARRY;

		if (bound > 0xFF)
			st->reg.F |= FLAG_CARRY;

		st->reg.A = (uint8_t)bound;

		if (st->reg.A == 0)
			st->reg.F |= FLAG_ZERO;

		break;
	}
		// SUB
	case 2:
	{
		DEBUG_OPCODES("SUB A, %s\n", resolve_register_name(z));

		int16_t bound = st->reg.A - *reg;

		st->reg.F = FLAG_SUBSTRACTION;

		if ((st->reg.A & 0xF) < (bound & 0xF))
			st->reg.F |= FLAG_HALF_CARRY;

		if (bound < 0)
			st->reg.F |= FLAG_CARRY;

		st->reg.A = (uint8_t)bound;

		if (st->reg.A == 0)
			st->reg.F |= FLAG_ZERO;

		break;
	}
		// SBC A,
	case 3:
	{
		DEBUG_OPCODES("SBC A, %s\n", resolve_register_name(z));

		int16_t bound = st->reg.A - *reg - (st->reg.F & FLAG_CARRY ? 1 : 0);

		st->reg.F = FLAG_SUBSTRACTION;
		if (bound < 0)
			st->reg.F |= FLAG_CARRY | FLAG_HALF_CARRY;

		st->reg.A = (uint8_t)bound;

		if (st->reg.A == 0)
			st->reg.F |= FLAG_ZERO;

		break;
	}
		// AND
	case 4:
		DEBUG_OPCODES("AND A, %s\n", resolve_register_name(z));

		st->reg.A &= *reg;
		st->reg.F = FLAG_NONE;
		st->reg.F |= FLAG_HALF_CARRY;
		if (st->reg.A == 0)
			st->reg.F |= FLAG_ZERO;

		break;
		// XOR
	case 5:
		DEBUG_OPCODES("XOR A, %s\n", resolve_register_name(z));
		st->reg.A ^= *reg;
		st->reg.F = FLAG_NONE;
		if (st->reg.A == 0)
			st->reg.F |= FLAG_ZERO;

		break;
		// OR
	case 6:
		DEBUG_OPCODES("OR A, %s\n", resolve_register_name(z));

		st->reg.A |= *reg;
		st->reg.F = FLAG_NONE;
		if (st->reg.A == 0)
			st->reg.F |= FLAG_ZERO;
		break;
		// CP
	case 7:
	{
		DEBUG_OPCODES("CP A, %s\n", resolve_register_name(z));

		int16_t bound = st->reg.A - *reg;
		uint8_t i = 0;
		st->reg.F = FLAG_SUBSTRACTION;

		if ((bound & 0xF) > (st->reg.A & 0xF))
			st->reg.F |= FLAG_HALF_CARRY;

		if (bound < 0)
			st->reg.F |= FLAG_CARRY;

		i = (uint8_t)bound;

		if (i == 0)
			st->reg.F |= FLAG_ZERO;

		break;
	}
	}

	if (is_hl)
		return 2;
	else
		return 1;
}

// Conditionnal return -- RET cc[y]
static int8_t handle_no_extra_x_3_z_0(uint8_t y, uint8_t z, uint8_t p, uint8_t q, state* st, memory* mem) {
	switch(y) {
		// NZ
	case 0:
		DEBUG_OPCODES("RET NZ\n");

		if ((st->reg.F & FLAG_ZERO) == 0) {
			st->reg.PC = memory_read_word(mem, st->reg.SP);
			st->reg.SP += sizeof(uint16_t);
			return 5;
		}

		return 2;

		// Z
	case 1:
		DEBUG_OPCODES("RET Z\n");

		if ((st->reg.F & FLAG_ZERO) != 0) {
			st->reg.PC = memory_read_word(mem, st->reg.SP);
			st->reg.SP += sizeof(uint16_t);
			return 5;
		}

		return 2;

		// NC
	case 2:
		DEBUG_OPCODES("RET NC\n");

		if ((st->reg.F & FLAG_CARRY) == 0) {
			st->reg.PC = memory_read_word(mem, st->reg.SP);
			st->reg.SP += sizeof(uint16_t);
			return 5;
		}

		return 2;

		// C
	case 3:
		DEBUG_OPCODES("RET C\n");

		if ((st->reg.F & FLAG_CARRY) != 0) {
			st->reg.PC = memory_read_word(mem, st->reg.SP);
			st->reg.SP += sizeof(uint16_t);
			return 5;
		}

		return 2;

		// LD (FF00+n), A
	case 4:
	{
		uint16_t addr = 0xFF00 + memory_read_byte(mem, st->reg.PC);
		st->reg.PC++;
		memory_write_byte(mem, addr, st->reg.A);
		DEBUG_OPCODES("LD (FF00+n=%X), A\n", addr);

		return 3;
	}
		// ADD SP, dd
	case 5:
	{
		int8_t content = memory_read_byte(mem, st->reg.PC);
		st->reg.PC++;
		st->reg.SP += content;
		uint8_t tmp = st->reg.SP ^ content ^ ((st->reg.SP + content) & 0xFF);

		DEBUG_OPCODES("ADD SP, %d\n", content);

		st->reg.F = FLAG_NONE;

		if (tmp & 0x100)
			st->reg.F |= FLAG_CARRY;

		if (tmp & 0x10)
			st->reg.F |= FLAG_HALF_CARRY;


		return 4;
	}
		// LD A, (FF00+n)
	case 6:
	{
		uint16_t addr = 0xFF00 + memory_read_byte(mem, st->reg.PC);
		st->reg.PC++;
		st->reg.A = memory_read_byte(mem, addr);
		DEBUG_OPCODES("LD A, (FF00+n=%X)\n", addr);

		return 3;
	}
		// LD HL, SP+dd
	case 7:
	{
		int8_t content = memory_read_byte(mem, st->reg.PC);
		uint8_t dd = st->reg.SP + content;
		st->reg.PC++;

		DEBUG_OPCODES("LD HL, SP+dd=%d\n", content);

		st->reg.H = dd >> 8;
		st->reg.L = dd;

		uint8_t tmp = st->reg.SP ^ content ^ dd;

		st->reg.F = FLAG_NONE;

		if (tmp & 0x100)
			st->reg.F |= FLAG_CARRY;

		if (tmp & 0x10)
			st->reg.F |= FLAG_HALF_CARRY;

		return 3;
	}
	}

	return -1;
}

// POP & various ops
static int8_t handle_no_extra_x_3_z_1(uint8_t y, uint8_t z, uint8_t p, uint8_t q, state* st, memory* mem) {
	// POP rp2[p]
	if (q == 0) {
		uint8_t *first = NULL;
		uint8_t *second = NULL;

		DEBUG_OPCODES("POP %s\n", resolve_register_pairs_v2_name(p));

		resolve_register_pairs_v2(st, p, &first, &second);
		*second = memory_read_byte(mem, st->reg.SP);
		*first = memory_read_byte(mem, st->reg.SP + 1);
		st->reg.SP += 2;
		return 3;
	} else {
		switch (p) {
			// RET
		case 0:
			DEBUG_OPCODES("RET\n");

			st->reg.PC = memory_read_word(mem, st->reg.SP);
			st->reg.SP += 2;

			return 4;

			// RETI
		case 1:
			DEBUG_OPCODES("RETI\n");

			st->reg.PC = memory_read_word(mem, st->reg.SP);
			st->reg.SP += 2;
			st->irq_master = 1;

			return 4;

			// JP HL
		case 2:
			DEBUG_OPCODES("JP HL\n");

			st->reg.PC = (st->reg.H << 8) + st->reg.L;
			return 1;

			// LD SP, HL
		case 3:
			DEBUG_OPCODES("LD SP, HL\n");

			st->reg.SP = (st->reg.H << 8) + st->reg.L;
			return 2;
		}
	}

	return -1;
}

// Conditionnal jump -- JP cc[y], nn
static int8_t handle_no_extra_x_3_z_2(uint8_t y, uint8_t z, uint8_t p, uint8_t q, state* st, memory* mem) {
	switch(y) {
		// NZ
	case 0:
	{
		uint16_t addr = memory_read_word(mem, st->reg.PC);
		DEBUG_OPCODES("JP NZ, %X\n", addr);

		if ((st->reg.F & FLAG_ZERO) == 0) {
			DEBUG_OPCODES("Jump taken\n");
			st->reg.PC = addr;
			return 4;
		} else {
			st->reg.PC += 2;
		}

		return 3;
	}
		// Z
	case 1:
	{
		uint16_t addr = memory_read_word(mem, st->reg.PC);
		DEBUG_OPCODES("JP Z, %X\n", addr);
		if ((st->reg.F & FLAG_ZERO) != 0) {
			st->reg.PC = addr;
			return 4;
		} else {
			st->reg.PC += 2;
		}

		return 3;
	}
		// NC
	case 2:
	{
		uint16_t addr = memory_read_word(mem, st->reg.PC);
		DEBUG_OPCODES("JP NC, %X\n", addr);
		if ((st->reg.F & FLAG_CARRY) == 0) {
			st->reg.PC = addr;
			return 4;
		} else {
			st->reg.PC += 2;
		}

		return 3;
	}
		// C
	case 3:
	{
		uint16_t addr = memory_read_word(mem, st->reg.PC);
		DEBUG_OPCODES("JP C, %X\n", addr);
		if ((st->reg.F & FLAG_CARRY) != 0) {
			st->reg.PC = addr;
			return 4;
		} else {
			st->reg.PC += 2;
		}

		return 3;
	}
        // LD (FF00+C), A
    case 4:
		DEBUG_OPCODES("LD (FF00+C), A\n");

        memory_write_byte(mem, 0xFF00 + st->reg.C, st->reg.A);
        return 2;

        // LD (nn), A
    case 5:
	{
		uint16_t addr = memory_read_word(mem, st->reg.PC);
		st->reg.PC += 2;
		memory_write_byte(mem, addr, st->reg.A);
		DEBUG_OPCODES("LD (nn=%X), A\n", addr);

		return 4;
	}
        // LD A, (FF00+C)
    case 6:
		DEBUG_OPCODES("LD A, (FF00+C)\n");

        st->reg.A = memory_read_byte(mem, 0xFF00 + st->reg.C);
        return 2;

        // LD A, (nn)
    case 7:

	{
		uint16_t addr = memory_read_word(mem, st->reg.PC);
		st->reg.PC += 2;
		st->reg.A = memory_read_byte(mem, addr);
		DEBUG_OPCODES("LD A, (nn=%x)\n", addr);

		return 4;
	}
    }

	return -1;
}

// Assorted operations
static int8_t handle_no_extra_x_3_z_3(uint8_t y, uint8_t z, uint8_t p, uint8_t q, state* st, memory* mem) {
    switch(y) {
        // JP nn
    case 0:
		st->reg.PC = memory_read_word(mem, st->reg.PC);
		DEBUG_OPCODES("JP %X\n", st->reg.PC);

		return 4;

        // 0xCB prefix
    case 1:
        ERROR("Should never be here !\n");
		return -1;

        // OUT (n), A -- d not exists in GB
    case 2:
		ERROR("OUT (n), A is not available on GB\n");
        return -1;

        // IN A, (n) -- do not exists in GB
    case 3:
		ERROR("IN (n), A is not available on GB\n");
        return -1;

        // EX (SP), HL -- do not exists in GB
    case 4:
		ERROR("EX (SP), HL is not available on GB\n");
        return -1;

        // EX DE, HL -- do not exists in GB
    case 5:
		ERROR("EX DE, HL is not available on GB\n");
        return -1;

        // DI
    case 6:
		DEBUG_OPCODES("DI\n");
		st->irq_master = 0;
        return 1;

        // EI
    case 7:
		DEBUG_OPCODES("EI\n");
		st->irq_master = 1;
        return 1;
    }

	return -1;
}

// Condtionnal call -- CALL cc[y], nn
static int8_t handle_no_extra_x_3_z_4(uint8_t y, uint8_t z, uint8_t p, uint8_t q, state* st, memory* mem) {

	DEBUG_OPCODES("CALL %s, %X\n", resolve_cc(y), memory_read_word(mem, st->reg.PC));

    switch(y) {
		// NZ
	case 0:
		if ((st->reg.F & FLAG_ZERO) == 0) {
            st->reg.SP -= 2;
            memory_write_word(mem, st->reg.SP, st->reg.PC + 2);
			st->reg.PC = memory_read_word(mem, st->reg.PC);

			return 6;
		} else {
            st->reg.PC += sizeof(uint16_t);
            return 3;
        }

		// Z
	case 1:
		if ((st->reg.F & FLAG_ZERO) != 0) {
            st->reg.SP -= 2;
            memory_write_word(mem, st->reg.SP, st->reg.PC + 2);
			st->reg.PC = memory_read_word(mem, st->reg.PC);

			return 6;
		} else {
            st->reg.PC += sizeof(uint16_t);
            return 3;
        }

		// NC
	case 2:
		if ((st->reg.F & FLAG_CARRY) == 0) {
            st->reg.SP -= 2;
            memory_write_word(mem, st->reg.SP, st->reg.PC + 2);
			st->reg.PC = memory_read_word(mem, st->reg.PC);

			return 6;
		} else {
            st->reg.PC += sizeof(uint16_t);
            return 3;
        }

		// C
	case 3:
		if ((st->reg.F & FLAG_CARRY) != 0) {
            st->reg.SP -= 2;
            memory_write_word(mem, st->reg.SP, st->reg.PC + 2);
			st->reg.PC = memory_read_word(mem, st->reg.PC);

			return 6;
		} else {
            st->reg.PC += sizeof(uint16_t);
            return 3;
        }

        // Not implemented in GB
    case 4:
    case 5:
    case 6:
    case 7:
		ERROR("Such an instruction for y = %d is not available on GB\n", y);
        return -1;
    }

	return -1;
}

// PUSH & various op
static int8_t handle_no_extra_x_3_z_5(uint8_t y, uint8_t z, uint8_t p, uint8_t q, state* st, memory* mem) {
    // PUSH rp2[p]
    if (q == 0) {
        uint8_t *first = NULL;
        uint8_t *second = NULL;

		DEBUG_OPCODES("PUSH %s\n", resolve_register_pairs_v2_name(p));


        resolve_register_pairs_v2(st, p, &first, &second);
        st->reg.SP -= 2;
        memory_write_byte(mem, st->reg.SP, *second);
        memory_write_byte(mem, st->reg.SP + 1, *first);

        return 4;
    } else {
        switch(p) {
            // CALL nn
        case 0:
            st->reg.SP -= 2;
            memory_write_word(mem, st->reg.SP, st->reg.PC + 2);
			st->reg.PC = memory_read_word(mem, st->reg.PC);

			DEBUG_OPCODES("CALL %X\n", st->reg.PC);

            return 6;

            // other prefixes -- do not exists in GB
        case 1:
        case 2:
        case 3:
			ERROR("Such an instruction for p = %d is not available on GB\n", p);
            return -1;
        }
    }

	return -1;
}

// Operate on accumulator and immediate operand
static int8_t handle_no_extra_x_3_z_6(uint8_t y, uint8_t z, uint8_t p, uint8_t q, state* st, memory* mem) {

    uint8_t val = memory_read_byte(mem, st->reg.PC);
    st->reg.PC++;

    switch(y) {
		// ADD A,
	case 0:
	{
		DEBUG_OPCODES("ADD A, %x\n", val);

		uint16_t bound = st->reg.A + val;

		st->reg.F = FLAG_NONE;

		if ((bound < 0xF) < (st->reg.A & 0xF))
			st->reg.F |= FLAG_HALF_CARRY;

		if (bound > 0xFF)
			st->reg.F |= FLAG_CARRY;

		st->reg.A = (uint8_t)bound;

		if (st->reg.A == 0)
			st->reg.F |= FLAG_ZERO;

		break;
	}
	// ADC A,
	case 1:
	{
		DEBUG_OPCODES("ADC A, %x\n", val);

		uint16_t bound = st->reg.A + val + (st->reg.F & FLAG_CARRY ? 1 : 0);

		st->reg.F = FLAG_NONE;

		if (bound > 0xF)
			st->reg.F |= FLAG_HALF_CARRY;

		if (bound > 0xFF)
			st->reg.F |= FLAG_CARRY;

		st->reg.A = (uint8_t)bound;

		if (st->reg.A == 0)
			st->reg.F |= FLAG_ZERO;

		break;
	}
	// SUB
	case 2:
	{
		DEBUG_OPCODES("SUB A, %x\n", val);

		int16_t bound = st->reg.A - val;

		st->reg.F = FLAG_SUBSTRACTION;
		if (bound < 0)
			st->reg.F |= FLAG_CARRY;

		if ((st->reg.A & 0xF) < (bound < 0xF))
			st->reg.F |= FLAG_HALF_CARRY;

		st->reg.A = (uint8_t)bound;

		if (st->reg.A == 0)
			st->reg.F |= FLAG_ZERO;

		break;
	}
	// SBC A,
	case 3:
	{
		DEBUG_OPCODES("SBC A, %x\n", val);

		int16_t bound = st->reg.A - val - (st->reg.F & FLAG_CARRY ? 1 : 0);

		st->reg.F = FLAG_SUBSTRACTION;
		if (bound < 0)
			st->reg.F |= FLAG_CARRY;

		if (bound < 0)
			st->reg.F |= FLAG_HALF_CARRY;

		st->reg.A = (uint8_t)bound;

		if (st->reg.A == 0)
			st->reg.F |= FLAG_ZERO;

		break;
	}
	// AND
	case 4:
		DEBUG_OPCODES("AND A, %x\n", val);

		st->reg.A &= val;
		st->reg.F = FLAG_NONE;
		if (st->reg.A == 0)
			st->reg.F |= FLAG_ZERO;
		st->reg.F |= FLAG_HALF_CARRY;

		break;
		// XOR
	case 5:
		DEBUG_OPCODES("XOR A, %x\n", val);

		st->reg.A ^= val;
		st->reg.F = FLAG_NONE;
		if (st->reg.A == 0)
			st->reg.F |= FLAG_ZERO;

		break;
		// OR
	case 6:
		DEBUG_OPCODES("OR A, %x\n", val);

		st->reg.A |= val;
		st->reg.F = FLAG_NONE;
		if (st->reg.A == 0)
			st->reg.F |= FLAG_ZERO;
		break;
		// CP
	case 7:
	{
		DEBUG_OPCODES("CP A, %x\n", val);

		int16_t bound = st->reg.A - val;
		uint8_t i = 0;

		st->reg.F = FLAG_SUBSTRACTION;
		if (bound < 0)
			st->reg.F |= FLAG_CARRY;

		if ((bound & 0xF) > (st->reg.A & 0xF))
			st->reg.F |= FLAG_HALF_CARRY;

		i = (uint8_t)bound;

		if (i == 0)
			st->reg.F |= FLAG_ZERO;

		break;
	}
	}

    return 2;
}

// Restart -- RST y*8
static int8_t handle_no_extra_x_3_z_7(uint8_t y, uint8_t z, uint8_t p, uint8_t q, state* st, memory* mem) {

	DEBUG_OPCODES("RST %X\n", y*8);

    st->reg.SP -= 2;
    memory_write_word(mem, st->reg.SP, st->reg.PC);
    st->reg.PC = y * 8;

    return 4;
}

static int8_t handle_no_extra_x_3(uint8_t y, uint8_t z, uint8_t p, uint8_t q, state* st, memory* mem) {
	switch(z) {
	case 0:
		return handle_no_extra_x_3_z_0(y, z, p, q, st, mem);
	case 1:
		return handle_no_extra_x_3_z_1(y, z, p, q, st, mem);
	case 2:
		return handle_no_extra_x_3_z_2(y, z, p, q, st, mem);
	case 3:
		return handle_no_extra_x_3_z_3(y, z, p, q, st, mem);
	case 4:
		return handle_no_extra_x_3_z_4(y, z, p, q, st, mem);
	case 5:
		return handle_no_extra_x_3_z_5(y, z, p, q, st, mem);
	case 6:
		return handle_no_extra_x_3_z_6(y, z, p, q, st, mem);
	case 7:
		return handle_no_extra_x_3_z_7(y, z, p, q, st, mem);
	}

	return -1;
}

// General function to decode an opcode.
// Based on http://www.z80.info/decoding.htm
static int8_t handle_OPCODE_general(z80_opcode opcode, state *st, memory* mem) {

	// Check for prefix byte (only 0xCB on gameboy)
	uint8_t has_extra_opcode = opcode == 0xCB;
	uint8_t extra_opcode = 0;
	if (has_extra_opcode) {
		extra_opcode = memory_read_byte(mem, st->reg.PC);
		st->reg.PC++;
	}

	// Extract bit variables
	uint8_t op = has_extra_opcode ? extra_opcode : opcode;
	uint8_t x = 0;
	uint8_t y = 0;
	uint8_t z = 0;
	uint8_t p = 0;
	uint8_t q = 0;

	x = (op & 0xC0) >> 6; // 0b11000000
	y = (op & 0x38) >> 3; // 0b00111000
	z = (op & 0x07) >> 0; // 0b00000111
	p = (op & 0x30) >> 4; // 0b00110000
	q = (op & 0x08) >> 3; // 0b00001000

	if (!has_extra_opcode) {
		switch(x) {
		case 0:
			return handle_no_extra_x_0(y, z, p, q, st, mem);
		case 1:
			return handle_no_extra_x_1(y, z, p, q, st, mem);
		case 2:
			return handle_no_extra_x_2(y, z, p, q, st, mem);
		case 3:
			return handle_no_extra_x_3(y, z, p, q, st, mem);
		}
	} else {
		switch(x) {
		case 0:
			return handle_roll(y, z, st, mem);
		case 1:
			return handle_bit(y, z, st, mem);
		case 2:
			return handle_res(y, z, st, mem);
		case 3:
			return handle_set(y, z, st, mem);
		}
	}

	return -1;
}

static void dump_states(state *st) {
	DEBUG_OPCODES("A = %X\n", st->reg.A);
	DEBUG_OPCODES("B = %X\n", st->reg.B);
	DEBUG_OPCODES("C = %X\n", st->reg.C);
	DEBUG_OPCODES("D = %X\n", st->reg.D);
	DEBUG_OPCODES("E = %X\n", st->reg.E);
	DEBUG_OPCODES("H = %X\n", st->reg.H);
	DEBUG_OPCODES("L = %X\n", st->reg.L);
	DEBUG_OPCODES("HL = %X\n", (st->reg.H << 8) + st->reg.L);
	DEBUG_OPCODES("SP = %X\n", st->reg.SP);
	DEBUG_OPCODES("PC = %X\n", st->reg.PC);
	DEBUG_OPCODES("F = %X\n", st->reg.F);
}

// Execute an opcode (separate function to not export opcodes tables)
int8_t opcodes_execute(z80_opcode opcode, state* st, memory* mem) {
	DEBUG_OPCODES("======================\n");
	DEBUG_OPCODES("Executing 0x%X\n", opcode);

	int8_t ret = handle_OPCODE_general(opcode, st, mem);

	dump_states(st);

	return ret;
}
