#ifndef __GUI_WINDOW_H
#define __GUI_WINDOW_H

#include "display.h"

extern Window gui_win1;
extern Window gui_win2;

Window *gui_handle_mouse_click(uint mouse_x, uint mouse_y);
void gui_redraw(Window *win, uint left_x, uint right_x, uint top_y, uint bottom_y);

typedef struct {
	uint left_x;
	uint right_x;
	uint top_y;
	uint bottom_y;
} rect;

#endif
