#ifndef __GUI_MOUSE_H
#define __GUI_MOUSE_H

void gui_mouse_move(int delta_x, int delta_y, uint mouse_button_down);
void gui_mouse_click();
void gui_mouse_unclick();
void gui_mouse_hide();
void gui_mouse_show();
void gui_save_mouse_buffer();

#endif
