#include <assert.h>
#include "opcodes.h"
#include "memory.h"
#include "emulator.h"

// Execute an opcode (separate function to not export opcodes tables)
int opcodes_execute(z80_opcodes *opcode, state* st, memory* mem) {
	assert(opcode > 0 && opcode < MAX_OPCODES);
	return handle_OPCODE_general(opcode, state, mem);
}

// Init opcodes if needed
void opcodes_init() {
}

// All opcode functions below.

// An opcode function. Take the current machine state and the memory and return
// the number of cycles taken by the instruction.

// General function to decode an opcode.
// Based on http://www.z80.info/decoding.htm
static uint8_t handle_OPCODE_general(z80_opcode opcode, state *st, memory* mem) {

	// Check for prefix byte (only 0xCB on gameboy)
	uint8_t has_extra_opcode = opcode == 0xCB;
	uint8_t extra_opcode = 0;
	if (has_extra_opcode)
		extra_opcode = memory_read_byte(mem, st.reg.PC);

	// Extract bit variables
	uint8_t op = has_extra_opcode ? extra_opcode : opcode;
	uint8_t x = 0;
	uint8_t y = 0;
	uint8_t z = 0;
	uint8_t p = 0;
	uint8_t q = 0;

	x = op & 0xC0 >> 6; // 0b11000000
	y = op & 0x38 >> 3; // 0b00111000
	z = op & 0x07 >> 0; // 0b00000111
	p = op & 0x30 >> 4; // 0b00110000
	q = op & 0x08 >> 3; // 0b00001000

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
}

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
}

// Extended instructions (rotation related)
static uint8_t handle_roll(uint8_t y, uint8_t z, state *st, memory* mem) {
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
		ci = *reg & 0x80 ? 1 : 0;
		co = *reg & 0x80 ? FLAG_CARRY : 0;
		*reg = (*reg << 1);
		break;

		// RRC
	case 1:
		ci = *reg & 0x1 ? 0x80 : 0;
		co = *reg & 0x1 ? FLAG_CARRY : 0;
		*reg = (*reg >> 1);
		break;

		// RL
	case 2:
		ci = st->reg.F & FLAG_CARRY ? 1 : 0;
		co = *reg & 0x80 ? FLAG_CARRY : 0;
		*reg = (*reg << 1);
		break;

		// RR
	case 3:
		ci = st->reg.F & FLAG_CARRY ? 0x80 : 0;
		co = *reg & 0x1 ? FLAG_CARRY : 0;
		*reg = (*reg >> 1);
		break;

		// SLA
	case 4:
		ci = 0;
		co = *reg & 0x80 ? FLAG_CARRY : 0;
		*reg = (*reg << 1);
		break;

		// SRA
	case 5:
		ci = *reg & 0x80;
		co = *reg & 0x1 ? FLAG_CARRY : 0;
		*reg = (*reg >> 1);
		break;

		// Special gameboy case SWAP
	case 6:
		ci = 0;
		co = 0;
		*reg = ((*reg&0xF) << 4) | ((*reg & 0xF0) >> 4);
		break;

		// SRL
	case 7:
		ci = 0;
		co = *reg & 0x1 ? FLAG_CARRY : 0;
		*reg = (*reg >> 1);
		break;
	}

    // General operations
	*reg += ci;
	st->reg.F = *reg ? 0 : FLAG_ZERO;
	st->reg.F |= co;

	// Write result to memory
	if (is_hl) {
		memory_write_byte(mem, (st->reg.H << 8) + st->reg.L, *reg);
		return 4;
	} else {
		return 2;
	}
}

// Check if a bit is set
static uint8_t handle_bit(uint8_t y, uint8_t z, state *st, memory* mem) {
	uint8_t* reg = resolve_register(z, st);

	// Get data from memory for (HL)
	uint8_t is_hl = reg == NULL;
	uint8_t hl_value = 0;
	if (is_hl) {
		hl_value = memory_read_byte(mem, (st->reg.H << 8) + st->reg.L);
		reg = &hl_value;
	}

	st->reg.F &= ~FLAG_OPERATION;
	st->reg.F |= FLAG_HALF_CARRY;
	st->reg.F |= *reg & (1 << y) ? 0 : FLAG_ZERO;

	if (is_hl)
		return 3;
	else
		return 2;
}

// Set to 0 a specific bit
static uint8_t handle_res(uint8_t y, uint8_t z, state *st, memory* mem) {
	uint8_t* reg = resolve_register(z, st);

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

// Set to 1 a specific bit
static uint8_t handle_set(uint8_t y, uint8_t z, state *st, memory* mem) {
	uint8_t* reg = resolve_register(z, st);

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

// Relative jumps and assorted ops
static uint8_t handle_no_extra_x_0_z_0(uint8_t y, uint8_t z, uint8_t p, uint8_t q, state* st, memory* mem) {
	switch (y) {
		// NOP
	case 0:
		return 1;

		// LD (nn), SP
	case 1:
		uint8_t nn = memory_read(mem, st->reg.PC);
		st->reg.PC++;

		memory_write_byte(mem, nn, st->reg.SP);

		return 5;

		// DJNZ d
	case 2:
		int8_t nn = memory_read(mem, st->reg.PC);
		st->reg.PC++;

		st->reg.B--;
		if (st->reg.B != 0) {
			st->reg.PC += nn;
			return 3;
		} else {
			return 2;
		}

		// JR d
	case 3:
		int8_t nn = memory_read(mem, st->reg.PC);
		st->reg.PC++;

		st->reg.PC += nn;
		return 3;

		// JR cc[y-4], d
	case 4:
	case 5:
	case 6:
	case 7:
		int8_t nn = memory_read(mem, st->reg.PC);
		st->reg.PC++;
		uint8_t do_jump = 0;

		// JR NZ, d
		if (y == 4)
			do_jump = st->reg.F & FLAG_ZERO == 0;
		// JR Z, d
		else if (y == 5)
			do_jump = st->reg.F & FLAG_ZERO != 0;
		// JR NC, d
		else if (y == 6)
			do_jump = st->reg.F & FLAG_CARRY != 0;
		// JR C, d
		else if (y == 7)
			do_jump = st->reg.F & FLAG_CARRY == 0;

		if (do_jump) {
			st->reg.PC += nn;
			return 3;
		} else {
			return 2;
		}
	}
}

// Select register using register pairs table
static void resolve_register_pairs(uint8_t p, uint8_t *first, uint8_t *second) {
	switch(p) {
	case 1:
		first = &(seq->req.B);
		second = &(seq->req.C);
		break;
	case 2:
		first = &(seq->req.D);
		second = &(seq->req.E);
		break;
	case 3:
		first = &(seq->req.H);
		second = &(seq->req.L);
		break;
	case 4:
		// Trick for SP to fit, suppose little-endianness
		first = &(seq->req.SP);
		second = &(seq->req.SP) + sizeof(uint8_t);
		break;
	}
}

// 16-bit load immediate/add
static uint8_t handle_no_extra_x_0_z_1(uint8_t y, uint8_t z, uint8_t p, uint8_t q, state* st, memory* mem) {
	uint8_t *first = NULL;
	uint8_t *second = NULL;
	resolve_register_pairs(p, first, second);

	// LD rp[p], nn
	if (q == 0) {
		*first = memory_write_byte(mem, st->reg.PC);
		st->reg.PC++;
		*second = memory_write_byte(mem, st->reg.PC);
		st->reg.PC++;

		return 3;
    // ADD HL, rp[p]
	} else {
		uint16_t hl = (st->reg.H << 8) + st->reg.L;
		uint16_t pr = (*first << 8) + *second;
		uint32_t res = hl + pr;

		if (res > 0xFFFF)
			st->reg.F |= FLAG_CARRY;
		else
			st->reg.F &= ~FLAG_CARRY;

		st->reg.H = (res & 0xFF) >> 8;
		st->reg.L = (res & 0xFF);
		return 3;
	}
}

// Indirect loading
static uint8_t handle_no_extra_x_0_z_2(uint8_t y, uint8_t z, uint8_t p, uint8_t q, state* st, memory* mem) {
	switch(q << 2 + p) {
		// LD (BC), A
	case 0:
		memory_write_byte(mem, (st->reg.B << 8) + st->reg.C, st->reg.A);
		return 2;
		// LD (DE), A
	case 1:
		memory_write_byte(mem, (st->reg.D << 8) + st->reg.E, st->reg.A);
		return 2;
		// LD (nn), HL
	case 2:
		uint16_t nn = memory_read_word(mem, st->reg.PC);
		st->reg.PC += 2;
		memory_write_byte(mem, nn, st->reg.H);
		memory_write_byte(mem, nn + sizeof(uint8_t), st->reg.L);

		return 5;
		// LD (nn), A
	case 3:
		uint16_t nn = memory_read_word(mem, st->reg.PC);
		st->reg.PC += 2;
		memory_write_byte(mem, nn, st->reg.A);

		return 4;
		// LD A, (BC)
	case 4:
		st->reg.A = memory_read_byte(mem, (st->reg.B << 8) + st->reg.C);
		st->reg.PC += 1;
		return 2;
		// LD A, (DE)
	case 5:
		st->reg.A = memory_read_byte(mem, (st->reg.D << 8) + st->reg.E);
		st->reg.PC += 1;
		return 2;
		// LD HL, (nn)
	case 6:
		uint16_t nn = memory_read_word(mem, st->reg.PC);
		st->reg.PC += 2;
		st->reg.H = memory_write_byte(mem, nn);
		st->reg.L = memory_write_byte(mem, nn);

		return 5;
		// LD A, (nn)
	case 7:
		uint16_t nn = memory_read_word(mem, st->reg.PC);
		st->reg.PC += 2;
		st->reg.A = memory_read_byte(mem, nn);

		return 4;
	}
}

// 16-bit INC/DEC
static uint8_t handle_no_extra_x_0_z_3(uint8_t y, uint8_t z, uint8_t p, uint8_t q, state* st, memory* mem) {
	uint8_t *first = NULL;
	uint8_t *second = NULL;
	resolve_register_pairs(p, first, second);

	uint16_t total = (*first << 8) + *second;

	// INC rp[p]
	if (q == 0)
		total++;
	// DEC rp[p]
	else
		total--;

	*first = total >> 8;
	*second = total;

	return 1;
}

// 8-bit INC
static uint8_t handle_no_extra_x_0_z_4(uint8_t y, uint8_t z, uint8_t p, uint8_t q, state* st, memory* mem) {
	uint8_t *reg = resolve_register(y, st);
	*reg++;

	st->reg.F = 0;
	if (*reg == 0)
		st->reg.F |= FLAG_ZERO;

	return 1;
}

// 8-bit DEC
static uint8_t handle_no_extra_x_0_z_5(uint8_t y, uint8_t z, uint8_t p, uint8_t q, state* st, memory* mem) {
	uint8_t *reg = resolve_register(y, st);
	*reg--;

	st->reg.F = 0;
	if (*reg == 0)
		st->reg.F |= FLAG_ZERO;

	return 1;
}

// 8-bit load immediate
static uint8_t handle_no_extra_x_0_z_6(uint8_t y, uint8_t z, uint8_t p, uint8_t q, state* st, memory* mem) {
	uint8_t *reg = resolve_register(y, st);
	uint8_t im = memory_read_byte(mem, st->reg.PC);
	st->reg.PC++;

	*reg = im;

	return 2;
}

// Assorted operations on accumulator/flags
static uint8_t handle_no_extra_x_0_z_7(uint8_t y, uint8_t z, uint8_t p, uint8_t q, state* st, memory* mem) {
	switch (y) {
		// RLCA
	case 0:
		uint16_t new = st->reg.A << 1;

		st->reg.F &= 0xEF;

		if (new & 0x100) {
			st->reg.F |= FLAG_CARRY;
			new += 1;
		}

		st->reg.A = new;
		break;
		// RRCA
	case 1:
		uint16_t new = st->reg.A >> 1;

		st->reg.F &= 0xEF;

		if (st->reg.A & 0x1) {
			st->reg.F |= FLAG_CARRY;
			new |= 0x80;
		}

		st->reg.A = new;
		break;
		// RLA
	case 2:
		uint16_t new = st->reg.A << 1;

		if (st->reg.F & FLAG_CARRY)
			new += 1;

		st->reg.F &= 0xEF;

		if (new & 0x100)
			st->reg.F |= FLAG_CARRY;

		st->reg.A = new;
		break;
		// RRA
	case 3:
		uint16_t new = st->reg.A >> 1;

		if (st->reg.F & FLAG_CARRY)
			new |= 0x80;

		st->reg.F &= 0xEF;

		if (st->reg.A & 0x1)
			st->reg.F |= FLAG_CARRY;

		st->reg.A = new;
		break;
		// DAA
		// This one from https://raw.githubusercontent.com/Two9A/jsGB/master/js/z80.js
	case 4:
		uint8_t a = st->reg.A;

		if (st->reg.F & FLAG_HALF_CARRY || st->reg.A & 0xF > 0x9)
			st->reg.A += 6;

		st->reg.F &= 0xEF;

		if (st->reg.F & FLAG_HALF_CARRY || st->reg.A > 0x99) {
			st->reg.A += 0x60;
			st->reg.F |= FLAG_CARRY;
		}

		break;
		// CPL
	case 5:
		st->reg.A = ~(st->reg.A);
		st->reg.F = 0;
		if (st->reg.A == 0)
			st->reg.F |= FLAG_ZERO;

		break;
		// SCF
	case 6:
		st->reg.F |= FLAG_CARRY;
		break;
		// CCF
	case 7:
		st->reg.F = st->reg.F ^ FLAG_CARRY;
		break;
	}

	return 1;
}

static uint8_t handle_no_extra_x_0(uint8_t y, uint8_t z, uint8_t p, uint8_t q, state* st, memory* mem) {
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
}

// Load reg_src into reg_dst
static uint8_t handle_no_extra_x_1(uint8_t y, uint8_t z, uint8_t p, uint8_t q, state* st, memory* mem) {
	uint8_t* reg_dst = resolve_register(z, st);
	uint8_t* reg_src = resolve_register(y, st);

	// Get data from memory for (HL)
	uint8_t is_hl_dst = reg_dst == NULL;
	uint8_t is_hl_src = reg_src == NULL;
	uint8_t hl_value = 0;

	// Special case of HALT instruction -- replace LD (HL), (HL)
	if (is_hl_dst && is_hl_src) {
		assert(false);
	}

	if (is_hl_dst) {
		hl_value = memory_read_byte(mem, (st->reg.H << 8) + st->reg.L);
		reg_dst = &hl_value;
	}

	if (is_hl_src) {
		hl_value = memory_read_byte(mem, (st->reg.H << 8) + st->reg.L);
		reg_src = &hl_value;
	}

	*reg_dst = *reg_src;

	if (is_hl_dst)
		memory_write_byte(mem, (st->reg.H << 8) + st->reg.L, *reg_dst);

	if (is_hl_src)
		memory_write_byte(mem, (st->reg.H << 8) + st->reg.L, *reg_src);

	if (is_hl_src || is_hl_dst)
		return 2;
	else
		return 1;
}

// Registers arithmetic
// TODO: refactor
static uint8_t handle_no_extra_x_2(uint8_t y, uint8_t z, uint8_t p, uint8_t q, state* st, memory* mem) {
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
		uint8_t old = st->reg.A;
		uint16_t bound = st->reg.A + *reg;

		if (bound > 0xFF)
			st->reg.F |= FLAG_CARRY;

		st->reg.A = (uint8_t)bound;

		if (st->reg.A == 0)
			st->reg.F |= FLAG_ZERO;

		if (((old & 0xF) + (*reg & 0xF)) & 0x10)
			st->reg.F |= FLAG_HALF_CARRY;
		break;

		// ADC A,
	case 1:
		uint8_t old = st->reg.A;
		uint16_t bound = st->reg.A + *reg + (st->reg.F & FLAG_CARRY ? 1 : 0);

		if (bound > 0xFF)
			st->reg.F |= FLAG_CARRY;

		st->reg.A = (uint8_t)bound;

		if (st->reg.A == 0)
			st->reg.F |= FLAG_ZERO;

		if (((old & 0xF) + (*reg & 0xF)) & 0x10)
			st->reg.F |= FLAG_HALF_CARRY;
		break;

		// SUB
	case 2:
		uint8_t old = st->reg.A;
		int16_t bound = st->reg.A - *reg;

		st->reg.F |= FLAG_SUBSTRACTION;
		if (bound < 0)
			st->reg.F |= FLAG_CARRY;

		st->reg.A = (uint8_t)bound;

		if (st->reg.A == 0)
			st->reg.F |= FLAG_ZERO;

		if (((old & 0xF) + (*reg & 0xF)) & 0x10)
			st->reg.F |= FLAG_HALF_CARRY;
		break;
		// SBC A,
	case 3:
		uint8_t old = st->reg.A;
		int16_t bound = st->reg.A - *reg - (st->reg.F & FLAG_CARRY ? 1 : 0);

		st->reg.F |= FLAG_SUBSTRACTION;
		if (bound < 0)
			st->reg.F |= FLAG_CARRY;

		st->reg.A = (uint8_t)bound;

		if (st->reg.A == 0)
			st->reg.F |= FLAG_ZERO;

		if (((old & 0xF) + (*reg & 0xF)) & 0x10)
			st->reg.F |= FLAG_HALF_CARRY;
		break;

		// AND
	case 4:
		st->reg.A &= *reg;
		if (st->reg.A == 0)
			st->reg.F |= FLAG_ZERO;

		break;
		// XOR
	case 5:
		st->reg.A ^= *reg;
		if (st->reg.A == 0)
			st->reg.F |= FLAG_ZERO;

		break;
		// OR
	case 6:
		st->reg.A |= *reg;
		if (st->reg.A == 0)
			st->reg.F |= FLAG_ZERO;
		break;
		// CP
	case 7:
		int16_t bound = old - *reg;

		st->reg.F |= FLAG_SUBSTRACTION;
		if (bound < 0)
			st->reg.F |= FLAG_CARRY;

		i = (uint8_t)bound;

		if (i == 0)
			st->reg.F |= FLAG_ZERO;

		if (((i & 0xF) + (st->reg.A & 0xF)) & 0x10)
			st->reg.F |= FLAG_HALF_CARRY;
		break;
	}

	if (is_hl)
		return 2;
	else
		return 1;
}

static uint8_t handle_no_extra_x_3(uint8_t y, uint8_t z, uint8_t p, uint8_t q, state* st, memory* mem) {
	// TODO
}
