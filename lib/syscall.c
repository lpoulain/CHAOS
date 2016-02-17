#include "libc.h"
#include "syscall.h"
#include "isr.h"
#include "process.h"

static void syscall_handler(registers_t regs);
extern void sysret();

void kprint(const char *txt) {
    printf_win(current_process->win, txt);
}

static void *syscalls[1] =
{
   &kprint,
};
uint num_syscalls = 1;

void syscall_handler2() {
   *((unsigned char *)0xb8000) = 'A';
   printf("Test\n");
}

void syscall_handler(registers_t regs)
{
//   *((unsigned char *)0xb8000) = 'A';

   // Firstly, check if the requested syscall number is valid.
   // The syscall number is found in EAX.
   if (regs.eax >= num_syscalls)
       return;

   // Get the required syscall location.
   void *location = syscalls[regs.eax];
   // We don't know how many parameters the function wants, so we just
   // push them all onto the stack in the correct order. The function will
   // use all the parameters it wants, and we can pop them all back off afterwards.
   int ret;
   asm volatile (" \
     push %1; \
     push %2; \
     push %3; \
     push %4; \
     push %5; \
     call *%6; \
     pop %%ebx; \
     pop %%ebx; \
     pop %%ebx; \
     pop %%ebx; \
     pop %%ebx; \
   " : "=a" (ret) : "r" (regs.edi), "r" (regs.esi), "r" (regs.edx), "r" (regs.ecx), "r" (regs.ebx), "r" (location));
   regs.eax = ret;
}

void init_syscalls()
{
   // Register our syscall handler.
   register_interrupt_handler (0x80, &syscall_handler);
}

DEFN_SYSCALL1(printf, 0, const char*);
