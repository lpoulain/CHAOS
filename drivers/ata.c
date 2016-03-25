#include "libc.h"
#include "kernel.h"
#include "display.h"

/*
unsigned char poll_drive(unsigned short drive, unsigned char arg) // drive could be 0x1F7 or 0x170 and args 0xA0 or 0xB0
{
   outportb(drive, arg);
   timer_wait(1);
   return inportb(drive) & (1 << 6);
}
*/

void write_hdd_lba28(unsigned char buffer[], unsigned int addr, uint8 sector_count, unsigned short drive, char is_slave)
{
	outportb(drive + 1, 0x00);
	outportb(drive + 2, sector_count);
	outportb(drive + 3, (unsigned char) ((addr) & 0xFF));
	outportb(drive + 4, (unsigned char)((addr >> 8) & 0xFF));
	outportb(drive + 5, (unsigned char)((addr >> 16) & 0xFF));
	outportb(drive + 6,  0xE0 | (is_slave << 4) | ((addr >> 24) & 0x0F));
	outportb(drive + 7, 0x30);

	while(!(inportb(drive + 7) & 0x08));

	for(int i = 0; i < sector_count * 256; i++)
	{
		unsigned short *val = (unsigned short*)(buffer + i*2);
    	outportw(drive, *val);
    }

    outportb(drive + 7, 0xE7);
}

void read_hdd_lba28(unsigned char buffer[], unsigned int addr, uint8 sector_count, unsigned short drive, char is_slave)
{
	outportb(drive + 1, 0x00);
	outportb(drive + 2, sector_count);
	outportb(drive + 3, (unsigned char) ((addr) & 0xFF));
	outportb(drive + 4, (unsigned char)((addr >> 8) & 0xFF));
	outportb(drive + 5, (unsigned char)((addr >> 16) & 0xFF));
	outportb(drive + 6,  0xE0 | (is_slave << 4) | ((addr >> 24) & 0x0F));
	outportb(drive + 7, 0x20);

	while(!(inportb(drive + 7) & 0x08));

	for (int i = 0; i < 256; i++)
	{
		unsigned short tmpword = inportw(drive);
		buffer[i * 2] = (unsigned char)tmpword;
		buffer[i * 2 + 1] = (unsigned char)(tmpword >> 8);
	}
}

extern void read_FAT12(unsigned char *ptr, uint nb);

void write_sector(unsigned char *buf, uint addr) {
	addr += 2049;
	uint sec_ct = 1;
	write_hdd_lba28(buf, addr, sec_ct, 0x1F0, 0); // primbase is 0x1F0
}

unsigned char * read_sector(unsigned char *buf, uint addr) {
   char addrbuf[4];
   char secbuf[4];
   addr += 2049;
//   uint addr = 2049;
   uint sec_ct = 1;

/*   debug_i("Start addr: ", addrbuf);
   getstr(addrbuf);
   printf("Sector count: ");
   getstr(secbuf);
*/
//   addr = atoi(addrbuf);
//   sec_ct = atoi(secbuf);
   
   read_hdd_lba28(buf, addr, sec_ct, 0x1F0, 0); // primbase is 0x1F0

//   dump_mem(&buf, 512, 0);
//   read_FAT12(&buf, 56);

   return buf;
}
