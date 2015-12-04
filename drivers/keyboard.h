#ifndef KEYBOARD_H
#define KEYBOARD_H

typedef void (*callback_fct)(unsigned char);

void keyboard_install();
void keyboard_set_callback(callback_fct cb);

#endif
