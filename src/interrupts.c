#include <stdlib.h>

#include "interrupts.h"
#include "emulator.h"
#include "memory.h"
#include "log.h"

interrupts *interrupts_init(memory *mem) {
	interrupts *ir = malloc(sizeof(interrupts));
	if (ir == NULL)
		ERROR("Unable to allocate memory for interrupts\n");

	ir->reg.enable = IRQ_NONE;
	ir->reg.flags = IRQ_NONE;

	memory_set_interrupts(mem, ir);
	return ir;
}

void interrupts_end(interrupts *ir) {
	free(ir);
}

void interrupts_process(interrupts *ir, state *st, memory* mem) {
	if (st->irq_master && ir->reg.enable && ir->reg.flags) {
		uint8_t cur_irq = ir->reg.enable & ir->reg.flags;

		uint8_t handled_exception = (cur_irq & IRQ_VBLANK) ||
			(cur_irq & IRQ_TIMER) ||
			(cur_irq & IRQ_JOYPAD);

		if (handled_exception) {
			WARN("Jump to interrupt\n");
			// Save pc on stack, disable interrupts
			st->irq_master = 0;
			st->reg.SP -= 2;
			memory_write_word(mem, st->reg.SP, st->reg.PC);
			st->clk += 3;


			// Ack IRQ & jump to handler
			if (cur_irq & IRQ_VBLANK) {
				ir->reg.flags &= ~IRQ_VBLANK;
				st->reg.PC = OFFSET_VBLANK;
			} else if (cur_irq & IRQ_LCD) {
				ir->reg.flags &= ~IRQ_LCD;
				st->reg.PC = OFFSET_LCD;
			} else if (cur_irq & IRQ_TIMER) {
				ir->reg.flags &= ~IRQ_TIMER;
				st->reg.PC = OFFSET_TIMER;
			} else if (cur_irq & IRQ_JOYPAD) {
				ir->reg.flags &= ~IRQ_JOYPAD;
				st->reg.PC = OFFSET_JOYPAD;
			}
		}
	}
}

void interrupts_raise(interrupts *ir, IRQ_FLAGS num) {
	ir->reg.flags |= num;
	DEBUG_INTERRUPTS("Raising %X\n", ir->reg.enable);
}
