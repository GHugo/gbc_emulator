#ifndef __OPCODES_H__
#define __OPCODES_H__

#include <stdint.h>

#include "memory.h"
#include "emulator.h"

typedef uint8_t z80_opcode;

void opcodes_init();
int8_t opcodes_execute(z80_opcode opcode, state* st, memory* mem);
#endif     // __OPCODES_H__
