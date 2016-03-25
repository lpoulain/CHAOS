// The kernel main function

#include "libc.h"
#include "kernel.h"
#include "kheap.h"
#include "virtualmem.h"
#include "keyboard.h"
#include "display.h"
#include "shell.h"
#include "process.h"
#include "descriptor_tables.h"
#include "syscall.h"

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

inline unsigned long inportl(unsigned short port)
{
   unsigned long result;
   __asm__ __volatile__("inl %%dx, %%eax" : "=a" (result) : "dN" (port));
   return result;
}

inline void outportl(unsigned short port, unsigned long data)
{
   __asm__ __volatile__("outl %%eax, %%dx" : : "d" (port), "a" (data));
}

uint16 inportw(int addr)
{
    uint16 ret;
    asm volatile("inw %%dx,%%ax \n" :"=a"(ret) :"d"(addr));
    return ret;
}

void outportw(int addr, uint16 val)
{
    asm volatile("outw %%ax,%%dx \n" : : "d"(addr), "a"(val));
}

/*
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
*/
void reboot()
{
    uint8 good = 0x02;
    while (good & 0x02)
        good = inportb(0x64);
    outportb(0x64, 0xFE);
}

uint initial_esp;
extern void init_scheduler();
extern void init_mouse();
extern void init_display();
extern uint boot_flags();
extern void init_debug();
extern void jump_usermode();
extern void init_syscalls();
extern void init_pci();
extern void init_network();

int main (uint esp) {
    // We save the first ESP pointer to have an idea of the
    // initial process base pointer
    initial_esp = esp;

    // We are initializing the heap
    init_kheap();
    init_display(boot_flags());
    init_processes();
    init_descriptor_tables();
    init_mouse();
    init_keyboard();
    init_debug();
    init_virtualmem();
    init_syscalls();
    init_PCI();
    init_network();
    init_tasking();
    init_scheduler();

    // Launch a new process
    int ret = fork();
    // Because we have two processes, this line will be called twice
//    jump_usermode();
//    current_process->function();
    shell();

    return 0;
}
