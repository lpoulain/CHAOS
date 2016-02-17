#ifndef __GUI_WINDOW_H
#define __GUI_WINDOW_H

#include "display.h"

extern Window gui_win1;
extern Window gui_win2;

typedef struct {
	uint left_x;
	uint right_x;
	uint top_y;
	uint bottom_y;
} rect;

Window *gui_handle_mouse_click(uint mouse_x, uint mouse_y);
void gui_redraw(Window *win, uint left_x, uint right_x, uint top_y, uint bottom_y);
int intersection(rect *rect1, rect *rect2, rect *intersect);

#endif
