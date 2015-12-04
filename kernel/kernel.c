#include "kernel.h"
#include "libc.h"
#include "keyboard.h"
#include "display.h"
#include "shell.h"

unsigned char inportb (unsigned short _port)
{
    unsigned char rv;
    __asm__ __volatile__ ("inb %1, %0" : "=a" (rv) : "dN" (_port));
    return rv;
}

void outportb (unsigned short _port, unsigned char _data)
{
    __asm__ __volatile__ ("outb %1, %0" : : "dN" (_port), "a" (_data));
}

void main () {
	const char *welcome_msg = "Welcome to CHAOS (CHrist, Another OS)! ";

    idt_install();
    isrs_install();
    irq_install();
    keyboard_install();

	__asm__ __volatile__ ("sti");

	puts(welcome_msg);
	start_shell();
}
