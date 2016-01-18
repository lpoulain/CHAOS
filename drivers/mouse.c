#include "libc.h"
#include "kernel.h"
#include "isr.h"
#include "display.h"
#include "vga.h"
#include "gui_mouse.h"

void mouse_wait(u8int a_type);

#define MOUSE_IRQ 12

#define MOUSE_PORT   0x60
#define MOUSE_STATUS 0x64
#define MOUSE_ABIT   0x02
#define MOUSE_BBIT   0x01
#define MOUSE_WRITE  0xD4
#define MOUSE_F_BIT  0x20
#define MOUSE_V_BIT  0x08

static u8int mouse_cycle = 0;
static u8int  mouse_byte[3];
static uint mouse_status;
static u8int cursor_buffer = ' ';
static u8int cursor_color_buffer = 0;

static int mouse_x = 40;
static int mouse_y = 12;

static void move(int delta_x, int delta_y) {
   print_c(cursor_buffer, mouse_y, mouse_x);
   delta_x /= 2;
   delta_y /= 2;
   

   mouse_x += delta_x;
   mouse_y += delta_y;

   if (mouse_x < 0) mouse_x = 0;
   if (mouse_x >= 80) mouse_x = 79;
   if (mouse_y < 0) mouse_y = 0;
   if (mouse_y >= 25) mouse_y = 24;

   print_c('X', mouse_y, mouse_x);
   cursor_buffer = ' ';
}

uint mouse_button_down = 0;

static void mouse_handler(registers_t regs) {
   int x_dir;
   int y_dir;

   u8int status = inportb(MOUSE_STATUS);
   while (status & MOUSE_BBIT) 
   {
      u8int mouse_in = inportb(MOUSE_PORT);
      if (status & MOUSE_F_BIT) 
      {
         switch (mouse_cycle) 
         {
            case 0:
               mouse_byte[0] = mouse_in;
//               draw_hex(mouse_byte[0], 0, 0);
               if (mouse_in & 0x1) {
                  if (!mouse_button_down) gui_mouse_click();
                  mouse_button_down = 1;
               }
               else {
                  if (mouse_button_down) gui_mouse_unclick();
                  mouse_button_down = 0;
               }
//               print_hex(mouse_in, 0, 60);
               if (!(mouse_in & MOUSE_V_BIT)) return;
               ++mouse_cycle;
               break;
            case 1:
               mouse_byte[1] = mouse_in;
               x_dir = mouse_in & 0xFF;
//               print_hex(mouse_in, 0, 65);
               if (mouse_byte[0] & 0x10) x_dir |= 0xFFFFFF00;
               ++mouse_cycle;
               break;
            case 2:
               mouse_byte[2] = mouse_in;
               y_dir = mouse_in & 0xFF;
//               print_hex(mouse_in, 0, 70);
               if (mouse_byte[0] & 0x20) y_dir |= 0xFFFFFF00;
               y_dir = -y_dir;
               /* We now have a full mouse packet ready to use */
               gui_mouse_move(x_dir, y_dir, mouse_button_down);

               if (mouse_byte[0] & 0x80 || mouse_byte[0] & 0x40) 
               {
                  
                  /* x/y overflow? bad packet! */
                  break;
               }
               mouse_cycle = 0;
               break;
         }
      }
      status = inportb(MOUSE_STATUS);
   }	
}

void mouse_write(u8int write) {
   mouse_wait(1);
   outportb(MOUSE_STATUS, MOUSE_WRITE);
   mouse_wait(1);
   outportb(MOUSE_PORT, write);
}

u8int mouse_read(void) {
   mouse_wait(0);
   char t = inportb(MOUSE_PORT);
   return t;
}

void mouse_wait(u8int a_type) {
   uint timeout = 100000;
   if (!a_type) {
      while (--timeout) {
         if ((inportb(MOUSE_STATUS) & MOUSE_BBIT) == 1) {
            return;
         }
      }
      return;
   } else {
      while (--timeout) {
         if (!((inportb(MOUSE_STATUS) & MOUSE_ABIT))) {
            return;
         }
      }
      return;
   }
}

void init_mouse() {
	mouse_wait(1);
	outportb(0x64, 0xA8);

	//Enable the interrupts
	mouse_wait(1);
	outportb(0x64, 0x20);
	mouse_wait(0);
	uint _status=(inportb(0x60) | 2);
	mouse_wait(1);
	outportb(0x64, 0x60);
	mouse_wait(1);
	outportb(0x60, _status);

	//Tell the mouse to use default settings
	mouse_write(0xF6);
	mouse_read();  //Acknowledge

	//Enable the mouse
	mouse_write(0xF4);
	mouse_read();  //Acknowledge	

	register_interrupt_handler(IRQ12, &mouse_handler);
}
