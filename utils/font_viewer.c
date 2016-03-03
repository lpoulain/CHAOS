#include "libc.h"
#include "display_vga.h"
#include "font.h"
#include "process.h"

void font_viewer() {
	uint pos[6] = {0, 0, 0, 0, 0, 0};
	uint exit_flag = 0;

	while (!exit_flag) {
		unsigned char c = getch();
		mouse_hide();

		if (c == '\n') exit_flag = 1;
		else {
			for (uint i=0; i<6; i++) {
				set_typeface(i);
				pos[i] += draw_typeface_font(c, pos[i] + 10, 20 * i + 10);
			}
		}

		mouse_show();
	}
}
