// The scheduler relies on IRQ 0 to be called at regular intervals and
// performs a context switch every time it's called

#include "libc.h"
#include "kernel.h"
#include "heap.h"
#include "virtualmem.h"
#include "display.h"
#include "process.h"
#include "isr.h"

void scheduler_phase(int hz)
{
    int divisor = 1193180 / hz;       /* Calculate our divisor */
    outportb(0x43, 0x36);             /* Set our command byte 0x36 */
    outportb(0x40, divisor & 0xFF);   /* Set low byte of divisor */
    outportb(0x40, divisor >> 8);     /* Set high byte of divisor */
}

uint timer_ticks = 0;

uint get_ticks() {
	return timer_ticks;
}

static void scheduler_handler(registers_t regs)
{
	timer_ticks++;
	switch_process();
}

void init_scheduler() {
	register_interrupt_handler(IRQ0, &scheduler_handler);
}
