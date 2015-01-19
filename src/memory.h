#ifndef __MEMORY_H__
#define __MEMORY_H__

#include <stdint.h>

typedef uint8_t memory;

uint8_t memory_read_byte(memory* mem, uint16_t addr);
uint16_t memory_read_word(memory* mem, uint16_t addr);

void memory_write_byte(memory* mem, uint16_t addr, uint8_t value);
void memory_write_word(memory* mem, uint16_t addr, uint16_t value);

#endif     // __MEMORY_H__
