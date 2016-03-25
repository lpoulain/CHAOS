#ifndef __KERNEL_H
#define __KERNEL_H

extern unsigned long inportl(unsigned short port);
extern void outportl(unsigned short port, unsigned long data);

extern uint16 inportw(int addr);
extern void outportw(int addr, uint16 val);

extern unsigned char inportb (unsigned short _port);
extern void outportb (unsigned short _port, unsigned char _data);

extern uint code;
extern uint data;
extern uint rodata;
extern uint bss;
extern uint debug_info;
extern uint end;

#endif
