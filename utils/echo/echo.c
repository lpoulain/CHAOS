#include "libc.h"
#include "syscall.h"

DEFN_SYSCALL1(printf, 0, const char*);

void _start(int argc, char **argv) {
    syscall_printf("Hello World!\n");
}
