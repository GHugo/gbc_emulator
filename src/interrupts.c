#include <stdlib.h>

#include "interrupts.h"
#include "emulator.h"
#include "memory.h"
#include "log.h"

interrupts *interrupts_init(memory *mem) {
	interrupts *ir = malloc(sizeof(interrupts));
	if (ir == NULL)
		ERROR("Unable to allocate memory for interrupts\n");

	ir->reg.mask = IRQ_NONE;
	ir->reg.flags = IRQ_VBLANK;

	memory_set_interrupts(mem, ir);
	return ir;
}

void interrupts_end(interrupts *ir) {
	free(ir);
}

void interrupts_process(interrupts *ir, state *st, memory* mem) {
	if (st->irq_master && ir->reg.mask && ir->reg.flags) {
		uint8_t cur_irq = ir->reg.mask & ir->reg.flags;

		// VBlank only for now
		if (cur_irq & IRQ_VBLANK) {

			// Save pc on stack, disable interrupts
			st->irq_master = 0;
			st->reg.SP -= 2;
			memory_write_word(mem, st->reg.SP, st->reg.PC);

			// Ack IRQ
			ir->reg.flags &= ~IRQ_VBLANK;

			// Update clk & jump to handler
			st->clk += 3;
			st->reg.PC = OFFSET_VBLANK;
		}
	}
}

void interrupts_raise(interrupts *ir, IRQ_FLAGS num) {
	ir->reg.flags |= num;
}
