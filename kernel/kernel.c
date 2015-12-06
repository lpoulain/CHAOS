#include "kernel.h"
#include "libc.h"
#include "keyboard.h"
#include "display.h"
#include "shell.h"
#include "process.h"

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

void switch_to_user_mode()
{
    // Set up our kernel stack.
//    set_kernel_stack(current_task->kernel_stack+KERNEL_STACK_SIZE);
    
    // Set up a stack structure for switching to user mode.
    asm volatile("  \
      cli; \
      mov $0x23, %ax; \
      mov %ax, %ds; \
      mov %ax, %es; \
      mov %ax, %fs; \
      mov %ax, %gs; \
                    \
       \
      mov %esp, %eax; \
      pushl $0x23; \
      pushl %esp; \
      pushf; \
      pushl $0x1B; \
      push $1f; \
      iret; \
    1: \
      "); 
      
}

void main () {

    idt_install();
    isrs_install();
    irq_install();

    keyboard_install();
    timer_install();

    __asm__ __volatile__ ("sti");

    init_processes();
    start_process(0);
    start_process(1);

//   switch_to_user_mode();

}
