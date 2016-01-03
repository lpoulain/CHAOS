#ifndef __KERNEL_H
#define __KERNEL_H

extern unsigned char inportb (unsigned short _port);
extern void outportb (unsigned short _port, unsigned char _data);
void outb(u16int port, u8int value);
u8int inb(u16int port);

#endif
