#ifndef __INTERRUPTS_H__
#define __INTERRUPTS_H__

#include <stdint.h>

typedef enum {
	IRQ_NONE   = 0x0,
	IRQ_VBLANK = (1 << 0),
	IRQ_LCD    = (1 << 1),
	IRQ_TIMER  = (1 << 2),
	IRQ_SERIAL = (1 << 3),
	IRQ_JOYPAD = (1 << 4)
} IRQ_FLAGS;

typedef enum {
	OFFSET_VBLANK = 0x40,
	OFFSET_LCD    = 0x48,
	OFFSET_TIMER  = 0x50,
	OFFSET_SERIAL = 0x58,
	OFFSET_JOYPAD = 0x60
} IRQ_OFFSETS;

typedef struct memory memory;
typedef struct state state;
typedef struct interrupts {
	struct {
		uint8_t mask;
		uint8_t flags;
	} reg;
} interrupts;

interrupts *interrupts_init(memory *mem);
void interrupts_end(interrupts *ir);
void interrupts_set_mmu(interrupts *ir);
void interrupts_process(interrupts *ir, state *st, memory* mem);
void interrupts_raise(interrupts *ir, IRQ_FLAGS num);

#endif     // __INTERRUPTS_H__
