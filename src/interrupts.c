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

		uint8_t handled_exception = (cur_irq & IRQ_VBLANK) ||
			(cur_irq & IRQ_TIMER);

		if (handled_exception) {

			// Save pc on stack, disable interrupts
			st->irq_master = 0;
			st->reg.SP -= 2;
			memory_write_word(mem, st->reg.SP, st->reg.PC);
			st->clk += 3;


			// Ack IRQ & jump to handler
			if (cur_irq & IRQ_VBLANK) {
				ir->reg.flags &= ~IRQ_VBLANK;
				st->reg.PC = OFFSET_VBLANK;
			} else if (cur_irq & IRQ_TIMER) {
				ir->reg.flags &= ~IRQ_TIMER;
				DEBUG_TIMER("Tick timer\n");
				st->reg.PC = OFFSET_TIMER;
			}
		}
	}
}

void interrupts_raise(interrupts *ir, IRQ_FLAGS num) {
	ir->reg.flags |= num;
}
