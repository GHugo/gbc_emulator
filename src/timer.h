#ifndef __TIMER_H__
#define __TIMER_H__

#include <stdint.h>

typedef struct interrupts interrupts;
typedef struct memory memory;
typedef struct timer {
	struct {
		uint8_t divider;
		uint8_t counter;
		uint8_t modulo;
		uint8_t control;
		uint8_t tick_divider;
		uint8_t tick_counter;
	} reg;
} timer;

timer *timer_init(memory *mem);
void timer_end(timer* t);
void timer_process(timer* t, interrupts *it, uint16_t clk);
#endif     // __TIMER_H__
