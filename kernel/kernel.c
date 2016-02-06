// The kernel main function

#include "libc.h"
#include "kernel.h"
#include "heap.h"
#include "virtualmem.h"
#include "keyboard.h"
#include "display.h"
#include "shell.h"
#include "process.h"
#include "descriptor_tables.h"

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

void outb(uint16 port, uint8 value)
{
    asm volatile ("outb %1, %0" : : "dN" (port), "a" (value));
}

uint8 inb(uint16 port)
{
    uint8 ret;
    asm volatile("inb %1, %0" : "=a" (ret) : "dN" (port));
    return ret;
}

void reboot()
{
    uint8 good = 0x02;
    while (good & 0x02)
        good = inb(0x64);
    outb(0x64, 0xFE);
}

uint initial_esp;
extern uint code;
extern uint bss;
extern uint end;
extern void init_scheduler();
extern void init_mouse();
extern void init_display();
extern uint boot_flags();
extern void init_debug();

int main (uint esp) {
    // We save the first ESP pointer to have an idea of the
    // initial process base pointer
    initial_esp = esp;

//    debug_i("code: ", (uint)&code);
//    debug_i("bss:  ", (uint)&bss);
//    debug_i("end:  ", (uint)&end);
    init_heap();
    init_display(boot_flags());
    init_processes();
    init_descriptor_tables();
    init_mouse();
    init_keyboard();
    init_debug();
    init_virtualmem();
    init_tasking();
    init_scheduler();

    // Launch a new process
    int ret = fork();
    // Because we have two processes, this line will be called twice
    shell();

    return 0;
}
