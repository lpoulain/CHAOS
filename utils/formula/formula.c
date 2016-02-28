#include "libc.h"
#include "syscall.h"

DEFN_SYSCALL1(printf, 0, const char*);

void print(int nb) {
    char text[256];
    int pos = 0;
    if (nb < 0) {
        text[pos++] = '-';
        nb = -nb;
    }

    int nb_ref = 1000000000;
    int leading_zero = 1;
    int digit, i;

    for (i=0; i<=9; i++) {
        if (nb >= nb_ref) {
            digit = nb / nb_ref;
            text[pos++] = '0' + digit;
            nb -= nb_ref * digit;
            leading_zero = 0;
        } else {
            if (!leading_zero) text[pos++] = '0';
        }
        nb_ref /= 10;
    }

    if (pos == 0) text[pos++] = '0';
    text[pos++] = '\n';
    text[pos++] = 0;

    syscall_printf((const char*)&text);
}

int atoi(char *str) {
    int res = 0, i;

    for (int i=0; str[i] != '\0'; i++)
        res = res*10 + str[i] - '0';

    return res;
}

void _start(int argc, char **argv) {
    int x = atoi(argv[0]);
    print(5 + (x-2) * 7);
    asm __volatile__ ("nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop;");
    asm __volatile__ ("nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop; nop;");    
}
