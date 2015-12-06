#include "kernel.h"
#include "display.h"

void timer_phase(int hz)
{
    int divisor = 1193180 / hz;       /* Calculate our divisor */
    outportb(0x43, 0x36);             /* Set our command byte 0x36 */
    outportb(0x40, divisor & 0xFF);   /* Set low byte of divisor */
    outportb(0x40, divisor >> 8);     /* Set high byte of divisor */
}

int timer_ticks = 0;
char timer = 0;

void timer_handler() {
	timer_ticks++;
	if (timer_ticks % 100 != 0) return;

	print_hex(timer++, 0, 40);
}

void timer_install() {
	irq_install_handler(0, timer_handler);
}

void stop_timer() {
	timer_phase(1);
	irq_install_handler(0, 0);
}
