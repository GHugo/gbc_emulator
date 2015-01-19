#include <assert.h>
#include <stdlib.h>
#include "opcodes.h"

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

	assert(0);
	return NULL;
}

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
static int8_t handle_bit(uint8_t y, uint8_t z, state *st, memory* mem) {
	uint8_t* reg = resolve_register(z, st);

	// Get data from memory for (HL)
	uint8_t is_hl = reg == NULL;
	uint8_t hl_value = 0;
	if (is_hl) {
		hl_value = memory_read_byte(mem, (st->reg.H << 8) + st->reg.L);
		reg = &hl_value;
	}

	st->reg.F &= ~FLAG_SUBSTRACTION;
	st->reg.F |= FLAG_HALF_CARRY;
	st->reg.F |= *reg & (1 << y) ? 0 : FLAG_ZERO;

	if (is_hl)
		return 3;
	else
		return 2;
}

// Set to 0 a specific bit
static int8_t handle_res(uint8_t y, uint8_t z, state *st, memory* mem) {
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
static int8_t handle_set(uint8_t y, uint8_t z, state *st, memory* mem) {
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
static int8_t handle_no_extra_x_0_z_0(uint8_t y, uint8_t z, uint8_t p, uint8_t q, state* st, memory* mem) {
	switch (y) {
		// NOP
	case 0:
		return 1;

		// LD (nn), SP
	case 1:
	{
		uint8_t nn = memory_read_byte(mem, st->reg.PC);
		st->reg.PC++;

		memory_write_byte(mem, nn, st->reg.SP);

		return 5;
	}
		// DJNZ d
	case 2:
	{
		int8_t nn = memory_read_byte(mem, st->reg.PC);
		st->reg.PC++;

		st->reg.B--;
		if (st->reg.B != 0) {
			st->reg.PC += nn;
			return 3;
		} else {
			return 2;
		}
	}
		// JR d
	case 3:
	{
		int8_t nn = memory_read_byte(mem, st->reg.PC);
		st->reg.PC++;

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

		// JR NZ, d
		if (y == 4)
			do_jump = (st->reg.F & FLAG_ZERO) == 0;
		// JR Z, d
		else if (y == 5)
			do_jump = (st->reg.F & FLAG_ZERO) != 0;
		// JR NC, d
		else if (y == 6)
			do_jump = (st->reg.F & FLAG_CARRY) != 0;
		// JR C, d
		else if (y == 7)
			do_jump = (st->reg.F & FLAG_CARRY) == 0;

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
static void resolve_register_pairs(state *st, uint8_t p, uint8_t *first, uint8_t *second) {
	switch(p) {
	case 1:
		first = &(st->reg.B);
		second = &(st->reg.C);
		break;
	case 2:
		first = &(st->reg.D);
		second = &(st->reg.E);
		break;
	case 3:
		first = &(st->reg.H);
		second = &(st->reg.L);
		break;
	case 4:
		// Trick for SP to fit, suppose little-endianness
		first = (uint8_t*)(&(st->reg.SP));
		second = (uint8_t*)(&(st->reg.SP)) + sizeof(uint8_t);
		break;
	}
}

// Select register using register pairs table version 2
static void resolve_register_pairs_v2(state *st, uint8_t p, uint8_t *first, uint8_t *second) {
	switch(p) {
	case 1:
		first = &(st->reg.B);
		second = &(st->reg.C);
		break;
	case 2:
		first = &(st->reg.D);
		second = &(st->reg.E);
		break;
	case 3:
		first = &(st->reg.H);
		second = &(st->reg.L);
		break;
	case 4:
		first = &(st->reg.A);
		second = &(st->reg.F);
		break;
	}
}

// 16-bit load immediate/add
static int8_t handle_no_extra_x_0_z_1(uint8_t y, uint8_t z, uint8_t p, uint8_t q, state* st, memory* mem) {
	uint8_t *first = NULL;
	uint8_t *second = NULL;
	resolve_register_pairs(st, p, first, second);

	// LD rp[p], nn
	if (q == 0) {
		*first = memory_read_byte(mem, st->reg.PC);
		st->reg.PC++;
		*second = memory_read_byte(mem, st->reg.PC);
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
static int8_t handle_no_extra_x_0_z_2(uint8_t y, uint8_t z, uint8_t p, uint8_t q, state* st, memory* mem) {
	switch((q << 2) + p) {
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
	{
		uint16_t nn = memory_read_word(mem, st->reg.PC);
		st->reg.PC += 2;
		memory_write_byte(mem, nn, st->reg.H);
		memory_write_byte(mem, nn + sizeof(uint8_t), st->reg.L);

		return 5;
	}
		// LD (nn), A
	case 3:
	{
		uint16_t nn = memory_read_word(mem, st->reg.PC);
		st->reg.PC += 2;
		memory_write_byte(mem, nn, st->reg.A);

		return 4;
	}
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
	{
		uint16_t nn = memory_read_word(mem, st->reg.PC);
		st->reg.PC += 2;
		st->reg.H = memory_read_byte(mem, nn);
		st->reg.L = memory_read_byte(mem, nn + sizeof(st->reg.H));

		return 5;
	}
		// LD A, (nn)
	case 7:
	{
		uint16_t nn = memory_read_word(mem, st->reg.PC);
		st->reg.PC += 2;
		st->reg.A = memory_read_byte(mem, nn);

		return 4;
	}
	}

	return -1;
}

// 16-bit INC/DEC
static int8_t handle_no_extra_x_0_z_3(uint8_t y, uint8_t z, uint8_t p, uint8_t q, state* st, memory* mem) {
	uint8_t *first = NULL;
	uint8_t *second = NULL;
	resolve_register_pairs(st, p, first, second);

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
static int8_t handle_no_extra_x_0_z_4(uint8_t y, uint8_t z, uint8_t p, uint8_t q, state* st, memory* mem) {
	uint8_t *reg = resolve_register(y, st);
	*reg += 1;

	st->reg.F = 0;
	if (*reg == 0)
		st->reg.F |= FLAG_ZERO;

	return 1;
}

// 8-bit DEC
static int8_t handle_no_extra_x_0_z_5(uint8_t y, uint8_t z, uint8_t p, uint8_t q, state* st, memory* mem) {
	uint8_t *reg = resolve_register(y, st);
	*reg -= 1;

	st->reg.F = 0;
	if (*reg == 0)
		st->reg.F |= FLAG_ZERO;

	return 1;
}

// 8-bit load immediate
static int8_t handle_no_extra_x_0_z_6(uint8_t y, uint8_t z, uint8_t p, uint8_t q, state* st, memory* mem) {
	uint8_t *reg = resolve_register(y, st);
	uint8_t im = memory_read_byte(mem, st->reg.PC);
	st->reg.PC++;

	*reg = im;

	return 2;
}

// Assorted operations on accumulator/flags
static int8_t handle_no_extra_x_0_z_7(uint8_t y, uint8_t z, uint8_t p, uint8_t q, state* st, memory* mem) {
	switch (y) {
		// RLCA
	case 0:
	{
		uint16_t new = st->reg.A << 1;

		st->reg.F &= 0xEF;

		if (new & 0x100) {
			st->reg.F |= FLAG_CARRY;
			new += 1;
		}

		st->reg.A = new;
		break;
	}
		// RRCA
	case 1:
	{
		uint16_t new = st->reg.A >> 1;

		st->reg.F &= 0xEF;

		if (st->reg.A & 0x1) {
			st->reg.F |= FLAG_CARRY;
			new |= 0x80;
		}

		st->reg.A = new;
		break;
	}
		// RLA
	case 2:
	{
		uint16_t new = st->reg.A << 1;

		if (st->reg.F & FLAG_CARRY)
			new += 1;

		st->reg.F &= 0xEF;

		if (new & 0x100)
			st->reg.F |= FLAG_CARRY;

		st->reg.A = new;
		break;
	}
		// RRA
	case 3:
	{
		uint16_t new = st->reg.A >> 1;

		if (st->reg.F & FLAG_CARRY)
			new |= 0x80;

		st->reg.F &= 0xEF;

		if (st->reg.A & 0x1)
			st->reg.F |= FLAG_CARRY;

		st->reg.A = new;
		break;
	}
		// DAA
		// This one from https://raw.githubusercontent.com/Two9A/jsGB/master/js/z80.js
	case 4:
	{
		if (st->reg.F & FLAG_HALF_CARRY || (st->reg.A & 0xF) > 0x9)
			st->reg.A += 6;

		st->reg.F &= 0xEF;

		if (st->reg.F & FLAG_HALF_CARRY || st->reg.A > 0x99) {
			st->reg.A += 0x60;
			st->reg.F |= FLAG_CARRY;
		}

		break;
	}
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

// Load reg_src into reg_dst
static int8_t handle_no_extra_x_1(uint8_t y, uint8_t z, uint8_t p, uint8_t q, state* st, memory* mem) {
	uint8_t* reg_dst = resolve_register(z, st);
	uint8_t* reg_src = resolve_register(y, st);

	// Get data from memory for (HL)
	uint8_t is_hl_dst = reg_dst == NULL;
	uint8_t is_hl_src = reg_src == NULL;
	uint8_t hl_value = 0;

	// Special case of HALT instruction -- replace LD (HL), (HL)
	if (is_hl_dst && is_hl_src) {
		assert(0);
		return -1;
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
	}
		// ADC A,
	case 1:
	{
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
	}
		// SUB
	case 2:
	{
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
	}
		// SBC A,
	case 3:
	{
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
	}
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
	{
		int16_t bound = st->reg.A - *reg;
		uint8_t i = 0;
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
	}

	if (is_hl)
		return 2;
	else
		return 1;
}

// Conditionnal return
static int8_t handle_no_extra_x_3_z_0(uint8_t y, uint8_t z, uint8_t p, uint8_t q, state* st, memory* mem) {
	switch(y) {
		// NZ
	case 0:
		if (st->reg.F & FLAG_ZERO) {
			st->reg.PC = memory_read_word(mem, st->reg.SP);
			st->reg.SP += sizeof(uint16_t);
			return 3;
		}

		return 1;

		// Z
	case 1:
		if ((st->reg.F & FLAG_ZERO) == 0) {
			st->reg.PC = memory_read_word(mem, st->reg.SP);
			st->reg.SP += sizeof(uint16_t);
			return 3;
		}

		return 1;

		// NC
	case 2:
		if (st->reg.F & FLAG_CARRY) {
			st->reg.PC = memory_read_word(mem, st->reg.SP);
			st->reg.SP += sizeof(uint16_t);
			return 3;
		}

		return 1;

		// C
	case 3:
		if ((st->reg.F & FLAG_CARRY) == 0) {
			st->reg.PC = memory_read_word(mem, st->reg.SP);
			st->reg.SP += sizeof(uint16_t);
			return 3;
		}

		return 1;

		// LD (FF00+n), A
	case 4:
	{
		uint16_t addr = 0xFF00 + memory_read_byte(mem, st->reg.PC);
		st->reg.PC++;
		memory_write_byte(mem, addr, st->reg.A);

		return 3;
	}
		// ADD SP, dd
	case 5:
	{
		int8_t content = memory_read_byte(mem, st->reg.PC);
		st->reg.PC++;

		st->reg.SP += content;
		return 4;
	}
		// LD A, (FF00+n)
	case 6:
	{
		uint16_t addr = 0xFF00 + memory_read_byte(mem, st->reg.PC);
		st->reg.PC++;
		st->reg.A = memory_read_byte(mem, addr);

		return 3;
	}
		// LD HL, SP+dd
	case 7:
	{
		int8_t dd = memory_read_byte(mem, st->reg.PC);
		st->reg.PC++;

		dd += st->reg.SP;
		st->reg.H = dd >> 8;
		st->reg.L = dd;
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

		resolve_register_pairs_v2(st, p, first, second);
		*second = memory_read_byte(mem, st->reg.SP);
		*first = memory_read_byte(mem, st->reg.SP + 1);
		st->reg.SP += 2;
		return 3;
	} else {
		switch (p) {
			// RET
		case 0:
			st->reg.PC = memory_read_word(mem, st->reg.SP);
			st->reg.SP += 2;

			return 3;

			// RET I
		case 1:
			st->reg.PC = memory_read_word(mem, st->reg.SP);
			st->reg.SP += 2;

			assert(0);
			return 3;

			// JP HL
		case 2:
			st->reg.PC = (st->reg.H << 8) + st->reg.L;
			return 1;

			// LD SP, HL
		case 3:
			st->reg.SP = (st->reg.H << 8) + st->reg.L;
			return 1;
		}
	}

	return -1;
}

// Condtionnal jump
static int8_t handle_no_extra_x_3_z_2(uint8_t y, uint8_t z, uint8_t p, uint8_t q, state* st, memory* mem) {
	switch(y) {
		// NZ
	case 0:
		if (st->reg.F & FLAG_ZERO) {
			uint16_t addr = memory_read_word(mem, st->reg.PC);
			st->reg.PC = addr;
			return 4;
		}

		return 5;

		// Z
	case 1:
		if ((st->reg.F & FLAG_ZERO) == 0) {
            uint16_t addr = memory_read_word(mem, st->reg.PC);
			st->reg.PC = addr;
			return 4;
		}

		return 5;

		// NC
	case 2:
		if (st->reg.F & FLAG_CARRY) {
            uint16_t addr = memory_read_word(mem, st->reg.PC);
			st->reg.PC = addr;
			return 4;
		}

		return 5;

		// C
	case 3:
		if ((st->reg.F & FLAG_CARRY) == 0) {
            uint16_t addr = memory_read_word(mem, st->reg.PC);
			st->reg.PC = addr;
			return 4;
		}

		return 5;

        // LD (FF00+C), A
    case 4:
        memory_write_byte(mem, 0xFF00 + st->reg.C, st->reg.A);
        return 2;

        // LD (nn), A
    case 5:
	{
        uint16_t addr = memory_read_word(mem, st->reg.PC);
        st->reg.PC += sizeof(uint16_t);
        memory_write_byte(mem, addr, st->reg.A);

        return 4;
	}
        // LD A, (FF00+C)
    case 6:
        st->reg.A = memory_read_byte(mem, 0xFF00 + st->reg.C);
        return 2;

        // LD A, (nn)
    case 7:
	{
        uint16_t addr = memory_read_word(mem, st->reg.PC);
        st->reg.PC += sizeof(uint16_t);
        st->reg.A = memory_read_byte(mem, addr);
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
        break;

        // 0xCB prefix
    case 1:
        assert(0);

        // OUT (n), A -- d not exists in GB
    case 2:
        assert(0);
        return -1;

        // IN A, (n) -- do not exists in GB
    case 3:
        assert(0);
        return -1;

        // EX (SP), HL -- do not exists in GB
    case 4:
        assert(0);
        return -1;

        // EX DE, HL -- do not exists in GB
    case 5:
        assert(0);
        return -1;

        // DI
    case 6:
        // Interupt related -- TODO
        assert(0);
        return -1;

        // EI
    case 7:
        // Interrupt related -- TODO
        assert(0);
        return -1;
    }

	return -1;
}

// Condtionnal call
static int8_t handle_no_extra_x_3_z_4(uint8_t y, uint8_t z, uint8_t p, uint8_t q, state* st, memory* mem) {
    switch(y) {
		// NZ
	case 0:
		if (st->reg.F & FLAG_ZERO) {
            st->reg.SP -= 2;
            memory_write_word(mem, st->reg.SP, st->reg.PC + 2);
			st->reg.PC = memory_read_word(mem, st->reg.PC);

			return 5;
		} else {
            st->reg.PC += sizeof(uint16_t);
            return 3;
        }

		// Z
	case 1:
		if ((st->reg.F & FLAG_ZERO) == 0) {
            st->reg.SP -= 2;
            memory_write_word(mem, st->reg.SP, st->reg.PC + 2);
			st->reg.PC = memory_read_word(mem, st->reg.PC);

			return 5;
		} else {
            st->reg.PC += sizeof(uint16_t);
            return 3;
        }

		// NC
	case 2:
		if (st->reg.F & FLAG_CARRY) {
            st->reg.SP -= 2;
            memory_write_word(mem, st->reg.SP, st->reg.PC + 2);
			st->reg.PC = memory_read_word(mem, st->reg.PC);

			return 5;
		} else {
            st->reg.PC += sizeof(uint16_t);
            return 3;
        }

		// C
	case 3:
		if ((st->reg.F & FLAG_CARRY) == 0) {
            st->reg.SP -= 2;
            memory_write_word(mem, st->reg.SP, st->reg.PC + 2);
			st->reg.PC = memory_read_word(mem, st->reg.PC);

			return 5;
		} else {
            st->reg.PC += sizeof(uint16_t);
            return 3;
        }

        // Not implemented in GB
    case 4:
    case 5:
    case 6:
    case 7:
        assert(0);
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

        resolve_register_pairs_v2(st, p, first, second);
        st->reg.SP -= 2;
        memory_write_byte(mem, st->reg.SP, *second);
        memory_write_byte(mem, st->reg.SP + 1, *first);

        return 3;
    } else {
        switch(p) {
            // CALL nn
        case 0:
            st->reg.SP -= 2;
            memory_write_word(mem, st->reg.SP, st->reg.PC + 2);
			st->reg.PC = memory_read_word(mem, st->reg.PC);

            return 5;

            // other prefixes -- do not exists in GB
        case 1:
        case 2:
        case 3:
            assert(0);
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
		uint8_t old = st->reg.A;
		uint16_t bound = st->reg.A + val;

		if (bound > 0xFF)
			st->reg.F |= FLAG_CARRY;

		st->reg.A = (uint8_t)bound;

		if (st->reg.A == 0)
			st->reg.F |= FLAG_ZERO;

		if (((old & 0xF) + (val & 0xF)) & 0x10)
			st->reg.F |= FLAG_HALF_CARRY;
		break;
	}
	// ADC A,
	case 1:
	{
		uint8_t old = st->reg.A;
		uint16_t bound = st->reg.A + val + (st->reg.F & FLAG_CARRY ? 1 : 0);

		if (bound > 0xFF)
			st->reg.F |= FLAG_CARRY;

		st->reg.A = (uint8_t)bound;

		if (st->reg.A == 0)
			st->reg.F |= FLAG_ZERO;

		if (((old & 0xF) + (val & 0xF)) & 0x10)
			st->reg.F |= FLAG_HALF_CARRY;
		break;
	}
	// SUB
	case 2:
	{
		uint8_t old = st->reg.A;
		int16_t bound = st->reg.A - val;

		st->reg.F |= FLAG_SUBSTRACTION;
		if (bound < 0)
			st->reg.F |= FLAG_CARRY;

		st->reg.A = (uint8_t)bound;

		if (st->reg.A == 0)
			st->reg.F |= FLAG_ZERO;

		if (((old & 0xF) + (val & 0xF)) & 0x10)
			st->reg.F |= FLAG_HALF_CARRY;
		break;
	}
	// SBC A,
	case 3:
	{
		uint8_t old = st->reg.A;
		int16_t bound = st->reg.A - val - (st->reg.F & FLAG_CARRY ? 1 : 0);

		st->reg.F |= FLAG_SUBSTRACTION;
		if (bound < 0)
			st->reg.F |= FLAG_CARRY;

		st->reg.A = (uint8_t)bound;

		if (st->reg.A == 0)
			st->reg.F |= FLAG_ZERO;

		if (((old & 0xF) + (val & 0xF)) & 0x10)
			st->reg.F |= FLAG_HALF_CARRY;
		break;
	}
	// AND
	case 4:
		st->reg.A &= val;
		if (st->reg.A == 0)
			st->reg.F |= FLAG_ZERO;

		break;
		// XOR
	case 5:
		st->reg.A ^= val;
		if (st->reg.A == 0)
			st->reg.F |= FLAG_ZERO;

		break;
		// OR
	case 6:
		st->reg.A |= val;
		if (st->reg.A == 0)
			st->reg.F |= FLAG_ZERO;
		break;
		// CP
	case 7:
	{
		int16_t bound = st->reg.A - val;
		uint8_t i = 0;

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
	}

    return 2;
}

// Restart
static int8_t handle_no_extra_x_3_z_7(uint8_t y, uint8_t z, uint8_t p, uint8_t q, state* st, memory* mem) {
    st->reg.SP -= 2;
    memory_write_word(mem, st->reg.SP, st->reg.PC);
    st->reg.PC = y * 8;

    return 3;
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
	if (has_extra_opcode)
		extra_opcode = memory_read_byte(mem, st->reg.PC);

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

	return -1;
}

// Execute an opcode (separate function to not export opcodes tables)
int8_t opcodes_execute(z80_opcode opcode, state* st, memory* mem) {
	return handle_OPCODE_general(opcode, st, mem);
}