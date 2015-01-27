#include <stdlib.h>
#include "log.h"
#include "memory.h"
#include "gpu.h"
#include "keyboard.h"

static uint8_t standard_bios[] = {
	0x31, 0xFE, 0xFF, 0xAF, 0x21, 0xFF, 0x9F, 0x32, 0xCB, 0x7C, 0x20, 0xFB, 0x21, 0x26, 0xFF, 0x0E,
	0x11, 0x3E, 0x80, 0x32, 0xE2, 0x0C, 0x3E, 0xF3, 0xE2, 0x32, 0x3E, 0x77, 0x77, 0x3E, 0xFC, 0xE0,
	0x47, 0x11, 0x04, 0x01, 0x21, 0x10, 0x80, 0x1A, 0xCD, 0x95, 0x00, 0xCD, 0x96, 0x00, 0x13, 0x7B,
	0xFE, 0x34, 0x20, 0xF3, 0x11, 0xD8, 0x00, 0x06, 0x08, 0x1A, 0x13, 0x22, 0x23, 0x05, 0x20, 0xF9,
	0x3E, 0x19, 0xEA, 0x10, 0x99, 0x21, 0x2F, 0x99, 0x0E, 0x0C, 0x3D, 0x28, 0x08, 0x32, 0x0D, 0x20,
	0xF9, 0x2E, 0x0F, 0x18, 0xF3, 0x67, 0x3E, 0x64, 0x57, 0xE0, 0x42, 0x3E, 0x91, 0xE0, 0x40, 0x04,
	0x1E, 0x02, 0x0E, 0x0C, 0xF0, 0x44, 0xFE, 0x90, 0x20, 0xFA, 0x0D, 0x20, 0xF7, 0x1D, 0x20, 0xF2,
	0x0E, 0x13, 0x24, 0x7C, 0x1E, 0x83, 0xFE, 0x62, 0x28, 0x06, 0x1E, 0xC1, 0xFE, 0x64, 0x20, 0x06,
	0x7B, 0xE2, 0x0C, 0x3E, 0x87, 0xF2, 0xF0, 0x42, 0x90, 0xE0, 0x42, 0x15, 0x20, 0xD2, 0x05, 0x20,
	0x4F, 0x16, 0x20, 0x18, 0xCB, 0x4F, 0x06, 0x04, 0xC5, 0xCB, 0x11, 0x17, 0xC1, 0xCB, 0x11, 0x17,
	0x05, 0x20, 0xF5, 0x22, 0x23, 0x22, 0x23, 0xC9, 0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B,
	0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D, 0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E,
	0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99, 0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC,
	0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E, 0x3c, 0x42, 0xB9, 0xA5, 0xB9, 0xA5, 0x42, 0x4C,
	0x21, 0x04, 0x01, 0x11, 0xA8, 0x00, 0x1A, 0x13, 0xBE, 0x20, 0xFE, 0x23, 0x7D, 0xFE, 0x34, 0x20,
	0xF5, 0x06, 0x19, 0x78, 0x86, 0x23, 0x05, 0x20, 0xFB, 0x86, 0x20, 0xFE, 0x3E, 0x01, 0xE0, 0x50
};

memory* memory_init(GB* rom) {
	memory* mem = malloc(sizeof(memory));
	if (mem == NULL)
		ERROR("Unable to allocate memory for memory structure.\n");

	mem->in_bios = 1;
	mem->bios = standard_bios;

	mem->rom = gbc_load_in_memory(rom);
	if (mem->rom == NULL)
		ERROR("Unable to load ROM into memory.\n");

	mem->external = calloc(0x2000, sizeof(uint8_t));
	if (mem->external == NULL)
		ERROR("Unable to allocate memory for external RAM.\n");

	mem->working = calloc(0x2000, sizeof(uint8_t));
	if (mem->working == NULL)
		ERROR("Unable to allocate memory for working RAM.\n");

	mem->zero = calloc(0x80, sizeof(uint8_t));
	if (mem->zero == NULL)
		ERROR("Unable to allocate memory for Zero page.\n");

	return mem;
}

void memory_end(memory *mem) {
	free(mem->external);
	free(mem->working);
	free(mem->zero);
	free(mem);
}

void memory_set_bios(memory* mem, uint8_t status) {
	mem->in_bios = status;
}

void memory_set_gpu(memory* mem, gpu *gp) {
	mem->gpu = gp->vram;
	mem->oam = gp->oam;
	mem->gp = gp;
}

uint8_t memory_read_byte(memory* mem, uint16_t addr) {
	void* offset = NULL;
	switch ((addr & 0xF000) >> 12) {
		// Cartridge ROM, bank 0
	case 0x0:
		// BIOS
		if ((addr & 0xFF00) == 0 && mem->in_bios)
			offset = mem->bios;
		else
			offset = mem->rom;
		break;
	case 0x1:
	case 0x2:
	case 0x3:
		offset = mem->rom;
		break;

		// Cartridge ROM, other banks
	case 0x4:
	case 0x5:
	case 0x6:
	case 0x7:
		offset = mem->rom;
		break;

		// Graphics RAM
	case 0x8:
	case 0x9:
		offset = mem->gpu - 0x8000;
		break;

		// Cartridge (External) RAM
	case 0xA:
	case 0xB:
		offset = mem->external - 0xA000;
		break;

		// Working RAM
	case 0xC:
	case 0xD:
		offset = mem->working - 0xC000;
		break;

		// Working RAM (shadow)
	case 0xE:
		offset = mem->working - 0xE000;
		break;

	case 0xF:
		// Working RAM (shadow)
		if (((addr & 0x0F00) >> 8) < 0xE)
			offset = mem->working - 0xE000;

		// Graphics: sprite information
		if (((addr & 0x0F00) >> 8) == 0xE) {
			if (((addr & 0x00F0) >> 4) < 0xA)
				offset = mem->oam - 0xFE00;
			else
				return 0;

			break;
		}

		// Zero page
		if (addr >= 0xFF80) {
			offset = mem->zero - 0xFF80;
			break;
		}

		// GPU registers

		// LCD control
		if (addr == 0xFF40) {
			DEBUG_MEMORY("Reading LCD control  = %X\n", mem->gp->reg.control);
			return mem->gp->reg.control;
		}

		// LCD Status
		if (addr == 0xFF41) {
			DEBUG_MEMORY("Reading LCD status  = %X\n", mem->gp->reg.status);
			return mem->gp->reg.status;
		}

		// Scroll Y
		if (addr == 0xFF42) {
			DEBUG_MEMORY("Reading GPU scroll_y = %X\n", mem->gp->reg.scroll_y);
			return mem->gp->reg.scroll_y;
		}

		// Scroll X
		if (addr == 0xFF43) {
			DEBUG_MEMORY("Reading GPU scroll_x = %X\n", mem->gp->reg.scroll_x);
			return mem->gp->reg.scroll_x;
		}

		// Scan line
		if (addr == 0xFF44) {
			DEBUG_MEMORY("Reading GPU scanline  = %X\n", mem->gp->reg.cur_line);
			return mem->gp->reg.cur_line;
		}

		// Bios mode
		if (addr == 0xFF50) {
			DEBUG_MEMORY("Reading in_bios = %X\n", mem->in_bios);
			return mem->in_bios;
		}

		// Keyboard Register
		if (addr == 0xFF00) {
			DEBUG_MEMORY("Reading keyboard register = %X\n", mem->kb->reg.joyp);
			return mem->kb->reg.joyp;
		}

		// I/O control
		WARN("I/O still not handled.\n");
		return 0;
	}

	if (offset == NULL)
		ERROR("Unknown addr 0x%X\n", addr);

	return *(uint8_t*)(offset + addr);
}

uint16_t memory_read_word(memory* mem, uint16_t addr) {
	return (uint16_t)((memory_read_byte(mem, addr)) + (memory_read_byte(mem, addr + 1) << 8));
}

void memory_write_byte(memory* mem, uint16_t addr, uint8_t value) {
		void* offset = NULL;
	switch ((addr & 0xF000) >> 12) {
		// Cartridge ROM, bank 0
	case 0x0:
		// BIOS
		if ((addr & 0xFF00) == 0 && mem->in_bios)
			offset = mem->bios;
		else
			offset = mem->rom;
		break;
	case 0x1:
	case 0x2:
	case 0x3:
		offset = mem->rom;
		break;

		// Cartridge ROM, other banks
	case 0x4:
	case 0x5:
	case 0x6:
	case 0x7:
		offset = mem->rom;
		break;

		// Graphics RAM
	case 0x8:
	case 0x9:
		offset = mem->gpu - 0x8000;
		break;

		// Cartridge (External) RAM
	case 0xA:
	case 0xB:
		offset = mem->external - 0xA000;
		break;

		// Working RAM
	case 0xC:
	case 0xD:
		offset = mem->working - 0xC000;
		break;

		// Working RAM (shadow)
	case 0xE:
		offset = mem->working - 0xE000;
		break;

	case 0xF:
		// Working RAM (shadow)
		if (((addr & 0x0F00) >> 8) < 0xE)
			offset = mem->working - 0xE000;

		// Graphics: sprite information
		if (((addr & 0x0F00) >> 8) == 0xE) {
			if (((addr & 0x00F0) >> 4) < 0xA) {
				DEBUG_GPU("Writing oam = %lu\n", (addr - 0xFE00) / sizeof(oam_data));
				offset = mem->oam - 0xFE00;
			} else {
				WARNING("Writing outside graphics zone 0x%X/\n", addr);
				return;
			}

			break;
		}

		// Zero page
		if (addr >= 0xFF80) {
			offset = mem->zero - 0xFF80;
			break;
		}

		// LCD control
		if (addr == 0xFF40) {
			DEBUG_MEMORY("Setting GPU LCD control to %x\n", value);
			mem->gp->reg.control = value;
			return;
		}

		// LCD Status
		if (addr == 0xFF41) {
			DEBUG_MEMORY("Setting GPU LCD_status to %x\n", value);
			mem->gp->reg.status = value;
			return;
		}

		// Scroll Y
		if (addr == 0xFF42) {
			DEBUG_MEMORY("Setting GPU scroll_y to %x\n", value);
			mem->gp->reg.scroll_y = value;
			return;
		}

		// Scroll X
		if (addr == 0xFF43) {
			DEBUG_MEMORY("Setting GPU scroll_x to %x\n", value);
			mem->gp->reg.scroll_x = value;
			return;
		}

		// Background Palette
		if (addr == 0xFF47) {
			DEBUG_MEMORY("Setting GPU background palette to %x\n", value);
			mem->gp->reg.bg_pal = value;
			return;
		}

		// Sprite Palette 0
		if (addr == 0xFF48) {
			DEBUG_MEMORY("Setting GPU sprite palette 0 to %x\n", value);
			mem->gp->reg.sp_pal_0 = value;
			return;
		}

		// Sprite Palette 1
		if (addr == 0xFF49) {
			DEBUG_MEMORY("Setting GPU sprite palette 1 to %x\n", value);
			mem->gp->reg.sp_pal_1 = value;
			return;
		}

		// Bios mode
		if (addr == 0xFF50) {
			DEBUG_MEMORY("Setting in_bios to %X\n", !value);
			mem->in_bios = !value;
			return;
		}

		// Keyboard Register
		if (addr == 0xFF00) {
			DEBUG_MEMORY("Setting keyboard register to %X\n", value);
			mem->kb->reg.joyp = (mem->kb->reg.joyp & 0xF) | (value & 0x30);
			return;
		}

		// I/O control
		WARN("I/O still not handled for 0x%X.\n", addr);
		return;
	}

	*(uint8_t*)(offset + addr) = value;
}

void memory_write_word(memory* mem, uint16_t addr, uint16_t value) {
	memory_write_byte(mem, addr, value & 0xFF);
	memory_write_byte(mem, addr + 1, value >> 8);
}
