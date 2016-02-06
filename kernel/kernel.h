#ifndef __KERNEL_H
#define __KERNEL_H

extern unsigned char inportb (unsigned short _port);
extern void outportb (unsigned short _port, unsigned char _data);
void outb(uint16 port, uint8 value);
uint8 inb(uint16 port);

#endif
