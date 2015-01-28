#ifndef __MEMORY_H__
#define __MEMORY_H__

#include <stdint.h>
#include <gbc_format.h>

typedef struct gpu gpu;
typedef struct keyboard keyboard;
typedef struct interrupts interrupts;

typedef struct memory {
	uint8_t in_bios;
	uint8_t mbc_mode;
	uint8_t rom_ram_mode;
	uint8_t ram_on;
	uint32_t ram_size;
	uint32_t rom_size;
	uint16_t mbc_cur_offset;
	uint16_t ram_cur_offset;

	uint8_t* bios;
	uint8_t* rom;
	uint8_t* gpu;
	uint8_t* external;
	uint8_t* working;
	uint8_t* oam;
	uint8_t* zero;
	gpu *gp;
	keyboard *kb;
	interrupts *ir;
} memory;

memory* memory_init(GB *rom);
void memory_end(memory* mem);

void memory_set_gpu(memory* mem, gpu* gp);
void memory_set_interrupts(memory* mem, interrupts* ir);

uint8_t memory_read_byte(memory* mem, uint16_t addr);
uint16_t memory_read_word(memory* mem, uint16_t addr);

void memory_write_byte(memory* mem, uint16_t addr, uint8_t value);
void memory_write_word(memory* mem, uint16_t addr, uint16_t value);

#endif     // __MEMORY_H__
