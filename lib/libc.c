///////////////////////////////////////////////////////////////////////////////////
// Standard C library functions
///////////////////////////////////////////////////////////////////////////////////

#include "libc.h"
#include "process.h"
#include "display.h"
#include "gui_screen.h"
#include "display_text.h"
#include "stdarg.h"
#include "debug_line.h"

char *strcpy(char *dest, const char *src) {
    char *save = dest;
    while(*dest++ = *src++);
    return save;	
}

char *strncpy(char *dest, const char *src, int n) {
    while (n-- && (*dest++ = *src++));
    return dest;
}

int strcmp(const char *s1, const char *s2)
{
    for ( ; *s1 == *s2; s1++, s2++)
  	if (*s1 == '\0') return 0;
    return ((*(unsigned char *)s1 < *(unsigned char *)s2) ? -1 : +1);
}

int strlen(const char *s) {
    const char *p = s;
    while (*s) ++s;
    return s - p;
}

int strnlen (const char *s, uint maxlen) {
  const char *e = s;
  uint n;

  for (n = 0; *e && n++ < maxlen; e++);
  return n;
}

int strncmp(const char *s1, const char *s2, uint n)
{
    for ( ; n > 0; s1++, s2++, --n)
    if (*s1 != *s2)
        return ((*(unsigned char *)s1 < *(unsigned char *)s2) ? -1 : +1);
    else if (*s1 == '\0')
        return 0;
    return 0;
}

// Copy len bytes from src to dest.
void memcpy(void *dest, const void *src, uint len)
{
    const uint8 *sp = (const uint8 *)src;
    uint8 *dp = (uint8 *)dest;
    for(; len != 0; len--) *dp++ = *sp++;
}

// Write len copies of val into dest.
void memset(void *dest, uint8 val, uint len)
{
    uint8 *temp = (uint8 *)dest;
    for ( ; len != 0; len--) *temp++ = val;
}

void lmemcpy(uint *dest, const uint *src, uint len) {
    for (; len != 0; len--) *dest++ = *src++;
}

void lmemset(uint *dest, uint val, uint len) {
    for (; len != 0; len--) *dest++ = val;
}

unsigned char getch() {
    current_process->flags |= PROCESS_POLLING;
    char c;

    for (;;) {
        if (current_process->buffer) {
            c = current_process->buffer;
            current_process->buffer = 0;
            return c;
        }
    }

    return current_process->buffer;
}

int atoi(char *str) {
    int res = 0; // Initialize result
  
    // Iterate through all characters of input string and
    // update result
    for (int i = 0; str[i] != '\0'; ++i)
        res = res*10 + str[i] - '0';
  
    // return result.
    return res;
}

void itoa(int nb, char *str) {
    if (nb < 0) {
        *str++ = '-';
        nb = -nb;
    }
    uint nb_ref = 1000000000;
    uint leading_zero = 1;
    uint digit;

    for (int i=0; i<=9; i++) {
        if (nb >= nb_ref) {
            digit = nb / nb_ref;
            *str++ = '0' + digit;
            nb -= nb_ref * digit;

            leading_zero = 0;
        } else {
            if (!leading_zero) *str++ = '0';
        }
        nb_ref /= 10;
    }

    if (leading_zero) *str++ = '0';

    *str = 0;
}

uint atoi_hex(char *str) {
    uint result = 0, mult = 1;
    int len = strlen(str)-1;
    do {
        if (str[len] >= '0' && str[len] <= '9') result += mult * (str[len] - '0');
        else if (str[len] >= 'A' && str[len] <= 'F') result += mult * (str[len] - 'A' + 10);
        else if (str[len] >= 'a' && str[len] <= 'f') result += mult * (str[len] - 'a' + 10);
        else {
//            debug_i("Unknown character", str[len]);
            return 0;
        }
        len--;
        mult *= 16;
    } while (len >= 0);

    return result;
}

uint umin(uint nb1, uint nb2) {
    if (nb1 <= nb2) return nb1;
    return nb2;
}

uint umax(uint nb1, uint nb2) {
    if (nb1 >= nb2) return nb1;
    return nb2;
}

int min(int nb1, int nb2) {
    if (nb1 <= nb2) return nb1;
    return nb2;
}

int max(int nb1, int nb2) {
    if (nb1 >= nb2) return nb1;
    return nb2;
}

///////////////////////////////////////////////////////////////////////////////////
// Debug functions
///////////////////////////////////////////////////////////////////////////////////

uint debug_pos = 0;

void debug_i(char *msg, uint nb) {
//    current_process->win->action->puts(current_process->win, msg);
//    current_process->win->action->puti(current_process->win, nb);
//    current_process->win->action->putcr(current_process->win);
    draw_string(msg, 0, debug_pos);
    draw_ptr((void*)nb, strlen(msg)*8, debug_pos);
    debug_pos += 8;
    if (debug_pos >= 480) debug_pos = 0;
}

void _printf(Window *win, const char *format, va_list args) {
    int x = 0;
    char *str;

    for (; *format != 0; ++format) {
        if (*format == '\n') {
            win->action->putcr(win);
//                    debug_pos += 8;
        } else if (*format == '%') {
            ++format;
            switch(*format) {
                case 'd':
                    win->action->putnb(win, va_arg( args, int ));
//                    x += draw_int(va_arg( args, int ), x, debug_pos) * 8;
                    break;
                case 's':
                    str = (char*)va_arg( args, int );
                    win->action->puts(win, str);
/*                    draw_string(str, x, debug_pos);
                    x += strlen(str) * 8;*/
                    break;
                case 'x':
                    win->action->puti(win, va_arg(args, unsigned int));
/*                    draw_ptr(va_arg(args, unsigned int), x, debug_pos);
                    x += 10*8;*/
                    break;
            }
        }
        else {
//            draw_font(*format, x, debug_pos);
            win->action->putc(win, *format);
//            x += 8;
        }
    }
}

extern uint8 is_debug();
extern uint8 switch_debug();
extern int debug_info_find_address(void *ptr, StackFrame *frame);
extern Window gui_win2;

void printf(const char *format, ...) {
    va_list args;
    
    va_start(args, format);
    _printf(window_debug, format, args);
}

void printf_win(Window *win, const char *format, ...) {
    va_list args;
    
    va_start(args, format);
    _printf(win, format, args);
}

void debug(char *msg) {
//    current_process->win->action->puts(current_process->win, msg);
    draw_string(msg, 0, debug_pos);
    debug_pos += 8;
    if (debug_pos >= 480) debug_pos = 0;
}

// Prints the stack trace. This is called by stack_dump() defined in
// kernel_entry.asm which determines the ESP and EBP pointers
// This function goes through the stack and finds the address of the
// EIP pointers for each function call
// You can find what function those pointers correspond to by looking at kernel.map
// which is generated by the makefile
void C_stack_dump(void *esp, void *ebp) {
    if (is_debug()) switch_debug();

    printf("ESP: %x, EBP: %x\n", (uint)esp, (uint)ebp);
    printf("Kernel stack:  %x-%x\n", (uint)&current_process->kernel_stack, (uint)&current_process->kernel_stack + PROCESS_STACK_SIZE);
    printf("Process stack: %x-%x\n", (uint)current_process->stack, (uint)current_process->stack + PROCESS_STACK_SIZE);

//    uint cs;
//    asm volatile("mov %%cs, %0" : "=r"(cs));

//    dump_mem(esp, 320, 1);
    uint ptr = (uint)esp;
/*    uint stack_start, stack_end;
    if ((uint)&current_process->kernel_stack <= ptr && ptr <= (uint)&current_process->kernel_stack + PROCESS_STACK_SIZE) {
        stack_start = (uint)&current_process->kernel_stack;
        stack_end = (uint)&current_process->kernel_stack + PROCESS_STACK_SIZE;
    }
    else {
        stack_start = (uint)current_process->stack;
        stack_end = (uint)current_process->stack + PROCESS_STACK_SIZE;
    }
*/
//       stack_start = (uint)current_process->stack;
//        stack_end = (uint)current_process->stack + PROCESS_STACK_SIZE;
/*
    stack_start = (uint)esp;
    stack_end = (uint)ebp + 0x100;*/


    uint fct_ptr, nb_lines, zeros;
    StackFrame frame;
    /*
    uint *tmp = (uint*)esp;

    while ( ((uint)&current_process->kernel_stack > *tmp || *tmp > (uint)&current_process->kernel_stack + PROCESS_STACK_SIZE) &&
            ((uint)current_process->stack > *tmp || *tmp > (uint)current_process->stack + PROCESS_STACK_SIZE) ) {
        tmp++;
    }
    ptr = (uint)tmp;
*/

//    while (ptr >= stack_start && ptr <= stack_end) {
    while ( ((uint)&current_process->kernel_stack <= ptr && ptr <= (uint)&current_process->kernel_stack + PROCESS_STACK_SIZE) ||
            ((uint)current_process->stack <= ptr && ptr <= (uint)current_process->stack + PROCESS_STACK_SIZE) ) {
        fct_ptr = *((uint*)ptr + 1);

        if (debug_line_find_address((void*)fct_ptr, &frame)) {
            debug_info_find_address((void*)fct_ptr, &frame);
            printf("[%x] %s (%s/%s at line %d)  \n", fct_ptr, frame.function, frame.path, frame.filename, frame.line_number);
        } else
            printf("[%x] n/a  \n", fct_ptr);
//        debug_i("Stack: ", fct_ptr);
        ptr = (uint)(*(uint*)ptr);
    }

//    dump_mem(esp, 320, 14);
//    for (;;);    
}

char ascii[256] = {
    '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.',
    '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.',
    ' ', '!', '"', '#', '$', '%', '&', '\'', '(', ')', '*', '+', ',', '-', '.', '/',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ':', ';', '<', '=', '>', '?',
    '@', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
    'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '[', '\\', ']', '^', '_',
    '`', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
    'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '{', '|', '}', '~', '.',
    '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.',
    '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.',
    '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.',
    '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.',
    '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.',
    '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.',
    '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.',
    '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.', '.'
};


void text_dump_mem(void *ptr, int nb_bytes, int row) {
    row = 14;
    unsigned char *addr = (unsigned char *)ptr;

    int offset = (uint)addr % 16;
    int i, j;

    text_print_ptr(addr - offset, row, 0);
    text_print("                                                  ", row, 10, YELLOW_ON_BLACK);
    for (i=0; i< 16-offset; i++) {
        text_print_hex2(*addr, row, offset * 3 + 12 + i*3);
        text_print_c(ascii[*addr++], row, 61 + i);
    }

    int nb_rows = (nb_bytes - 16 + offset) / 16 + 1;
    for (j=0; j<nb_rows; j++) {
        text_print_ptr(addr - 0, ++row, 0);
        text_print("                                                  ", row, 10, YELLOW_ON_BLACK);
        for (i=0; i<16; i++) {
            text_print_hex2(*addr, row, 12 + i*3);
            text_print_c(ascii[*addr++], row, 61 + i);
        }
    }
}

void gui_dump_mem(void *ptr, int nb_bytes, int row) {
    unsigned char *addr = (unsigned char *)ptr;

    int offset = (uint)addr % 16;
    int i, j;

    draw_ptr(addr - offset, 0, row*8);

    draw_string("                                                  ", 10*8, row*8);
    for (i=0; i< 16-offset; i++) {
        draw_hex2(*addr, (offset * 3 + 12 + i*3)*8, row*8);
        draw_char(ascii[*addr++], (61 + i)*8, row*8);
    }

    int nb_rows = (nb_bytes - 16 + offset) / 16 + 1;
    for (j=0; j<nb_rows; j++) {
        draw_ptr(addr - 0, 0, (++row)*8);
        draw_string("                                                  ", 10*8, row*8);
        for (i=0; i<16; i++) {
            draw_hex2(*addr, (12 + i*3)*8, row*8);
            draw_char(ascii[*addr++], (61 + i)*8, row*8);
        }
    }
}
