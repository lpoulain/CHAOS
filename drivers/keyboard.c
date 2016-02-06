#include "libc.h"
#include "kernel.h"
#include "keyboard.h"
#include "display.h"
#include "process.h"
#include "isr.h"

extern void stack_dump();

// US Keyboard layout
unsigned char kbdus[128] =
{
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8',	/* 9 */
  '9', '0', '-', '=', '\b',	/* Backspace */
  '\t',			/* Tab */
  'q', 'w', 'e', 'r',	/* 19 */
  't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',		/* Enter key */
    0,			/* 29   - Control */
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',	/* 39 */
 '\'', '`',   0,		/* Left shift */
 '\\', 'z', 'x', 'c', 'v', 'b', 'n',			/* 49 */
  'm', ',', '.', '/',   0,					/* Right shift */
  '*',
    0,	/* Alt */
  ' ',	/* Space bar */
    0,	/* Caps lock */
    0,	/* 59 - F1 key ... > */
    0,   0,   0,   0,   0,   0,   0,   0,
    0,	/* < ... F10 */
    0,	/* 69 - Num lock*/
    0,	/* Scroll Lock */
    0,	/* Home key */
    1,	/* Up Arrow */
    0,	/* Page Up */
  '-',
    3,	/* Left Arrow */
    0,
    4,	/* Right Arrow */
  '+',
    0,	/* 79 - End key*/
    2,	/* Down Arrow */
    0,	/* Page Down */
    0,	/* Insert Key */
    0,	/* Delete Key */
    0,   0,   0,
    0,	/* F11 Key */
    0,	/* F12 Key */
    0,	/* All other keys are undefined */
};

unsigned char kbdus_shift[128] =
{
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', /* 9 */
  '(', ')', '_', '+', '\b', /* Backspace */
  '\t',         /* Tab */
  'Q', 'W', 'E', 'R',   /* 19 */
  'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',     /* Enter key */
    0,          /* 29   - Control */
  'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', /* 39 */
 '\"', '~',   0,        /* Left shift */
 '|', 'Z', 'X', 'C', 'V', 'B', 'N',            /* 49 */
  'M', '<', '>', '?',   0,                  /* Right shift */
  '*',
    0,  /* Alt */
  ' ',  /* Space bar */
    0,  /* Caps lock */
    0,  /* 59 - F1 key ... > */
    0,   0,   0,   0,   0,   0,   0,   0,
    0,  /* < ... F10 */
    0,  /* 69 - Num lock*/
    0,  /* Scroll Lock */
    0,  /* Home key */
    1,  /* Up Arrow */
    0,  /* Page Up */
  '-',
    3,  /* Left Arrow */
    0,
    4,  /* Right Arrow */
  '+',
    0,  /* 79 - End key*/
    2,  /* Down Arrow */
    0,  /* Page Down */
    0,  /* Insert Key */
    0,  /* Delete Key */
    0,   0,   0,
    0,  /* F11 Key */
    0,  /* F12 Key */
    0,  /* All other keys are undefined */
};
uint shift_key_pressed = 0;

/* Handles the keyboard interrupt */
static void keyboard_handler(registers_t regs)
{
    unsigned char scancode;

    // Read the scan code of the key pressed
    scancode = inportb(0x60);

    // If the key has just been released
    if (scancode & 0x80)
    {
        // Handle the case where the user released the shift, alt or control key
        scancode &= ~0x80;
        if (scancode == 42 || scancode == 54) shift_key_pressed = 0;
    }
    else
    {
        // Handle the case where a pkey was pressed

        // Shift is pressed
        if (scancode == 42 || scancode == 54) {
            shift_key_pressed = 1;
            return;
        }

        // If we press the Esc key, print a stack trace
        if (scancode == 1) {
            stack_dump();
            return;
        }

        char c;
        if (shift_key_pressed) c = kbdus_shift[scancode];
        else c = kbdus[scancode];

        // If it is not an actionable key, do nothing
        if (c == 0) return;

        // If the user has pressed tab, switch the focus process
        if (c == '\t') {
            switch_window_focus();
            return;
        }

        // The keyboard handler does not process keystrokes per say
        // It looks at the process which has the focus
        // If that process has the PROCESS_POLLING flag turned on
        // it means it is waiting for keyboard input
        // The handler is thus filling the process buffer and flipping
        // off the PROCESS_POLLING flag
        
        Process *ps = get_process_focus();
        if (ps->flags | PROCESS_POLLING) {
            ps->buffer = c;
            ps->flags &= ~PROCESS_POLLING;
        }

    }
}

/* Installs the keyboard handler into IRQ1 */
void init_keyboard()
{
    register_interrupt_handler(IRQ1, &keyboard_handler);
}
