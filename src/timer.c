#include <stdlib.h>

#include "timer.h"
#include "memory.h"
#include "interrupts.h"
#include "log.h"

timer* timer_init(memory *mem) {
	timer *t = malloc(sizeof(timer));
	if (t == NULL)
		ERROR("Unable to allocate memory for timer.\n");

	t->reg.tick_divider = 0;
	t->reg.tick_counter = 0;

	t->reg.divider = 0;
	t->reg.counter = 0;
	t->reg.modulo = 0;
	t->reg.control = 0;

	memory_set_timer(mem, t);
	return t;
}

void timer_end(timer* t) {
	free(t);
}

void timer_process(timer* t, interrupts *ir , uint16_t clk) {
	if ((t->reg.control & 0x3) == 0)
		return;
	t->reg.tick_divider += clk;
	t->reg.tick_counter += clk;

	if (t->reg.tick_divider >= 64) {
		t->reg.tick_divider -= 64;
		t->reg.divider++;
	}

	uint16_t threshold = 0;
	switch(t->reg.control & 0x3) {
	case 0:
		threshold = 64 * 4;
		break;
	case 1:
		threshold = 1 * 4;
		break;
	case 2:
		threshold = 4 * 4;
		break;
	case 3:
		threshold = 16 * 4;
		break;
	}

	if (t->reg.tick_counter >= threshold) {
		if (t->reg.counter == 0xFF) {
			t->reg.counter = t->reg.modulo;
			interrupts_raise(ir, IRQ_TIMER);
		} else {
			t->reg.counter++;
		}
	}
}
