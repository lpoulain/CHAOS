#include "libc.h"
#include "kernel.h"
#include "keyboard.h"
#include "process.h"
#include "isr.h"

extern process *current_process;
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
    0,	/* Up Arrow */
    0,	/* Page Up */
  '-',
    0,	/* Left Arrow */
    0,
    0,	/* Right Arrow */
  '+',
    0,	/* 79 - End key*/
    0,	/* Down Arrow */
    0,	/* Page Down */
    0,	/* Insert Key */
    0,	/* Delete Key */
    0,   0,   0,
    0,	/* F11 Key */
    0,	/* F12 Key */
    0,	/* All other keys are undefined */
};

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
    }
    else
    {
        // Handle the case where a pkey was pressed

        // If we press the Esc key, print a stack trace
        if (scancode == 1) {
            stack_dump();
            return;
        }

        // If it is not an actionable key, do nothing
        if (kbdus[scancode] == 0) return;

        // If the user has pressed tab, switch the focus process
        if (kbdus[scancode] == '\t') {
            switch_process_focus();
            return;
        }

        // The keyboard handler does not process keystrokes per say
        // It looks at the process which has the focus
        // If that process has the PROCESS_POLLING flag turned on
        // it means it is waiting for keyboard input
        // The handler is thus filling the process buffer and flipping
        // off the PROCESS_POLLING flag
        process *ps = get_process_focus();
        if (ps->flags | PROCESS_POLLING) {
            ps->buffer = kbdus[scancode];
            ps->flags &= ~PROCESS_POLLING;
        }

    }
}

/* Installs the keyboard handler into IRQ1 */
void init_keyboard()
{
    register_interrupt_handler(IRQ1, &keyboard_handler);
}
