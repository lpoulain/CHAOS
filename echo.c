#include "libc.h"
#include "syscall.h"

DEFN_SYSCALL1(printf, 0, const char*);

void _start() {
	syscall_printf("It works!\n");
}
