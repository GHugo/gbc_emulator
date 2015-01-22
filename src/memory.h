#ifndef __MEMORY_H__
#define __MEMORY_H__

#include <stdint.h>
#include <gbc_format.h>

typedef struct gpu gpu;

typedef struct memory {
	uint8_t in_bios;
	uint8_t* bios;
	uint8_t* rom;
	uint8_t* gpu;
	uint8_t* external;
	uint8_t* working;
	uint8_t* sprites;
	uint8_t* zero;
	gpu *gp;
} memory;

memory* memory_init(GB *rom);
void memory_end(memory* mem);

void memory_set_bios(memory* mem, uint8_t status);
void memory_set_gpu(memory* mem, gpu* gp);

uint8_t memory_read_byte(memory* mem, uint16_t addr);
uint16_t memory_read_word(memory* mem, uint16_t addr);

void memory_write_byte(memory* mem, uint16_t addr, uint8_t value);
void memory_write_word(memory* mem, uint16_t addr, uint16_t value);

#endif     // __MEMORY_H__
