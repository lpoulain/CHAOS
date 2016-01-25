// These are the basic primitives to draw on a 640x480 monochrome screen

#include "libc.h"
#include "display.h"
#include "display_vga.h"
#include "gui_mouse.h"
#include "gui_screen.h"

window *gui_handle_mouse_click(uint mouse_x, uint mouse_y);
void bitarray_copy(const unsigned char *src_org, int src_offset, int src_len, unsigned char *dst_org, int dst_offset);

// 640x480 monochrome display

unsigned char cursor_large_white[32] = { 0x00, 0x00,
								   0x40, 0x00,
								   0x60, 0x00,
								   0x70, 0x00,
								   0x78, 0x00,
								   0x7C, 0x00,
								   0x7E, 0x00,
								   0x7F, 0x00,
								   0x7F, 0x80,
								   0x7C, 0x00,
								   0x6C, 0x00,
								   0x46, 0x00,
								   0x06, 0x00,
								   0x03, 0x00,
								   0x03, 0x00,
								   0x00, 0x00 };

unsigned char cursor_large[32] = { 0xC0, 0x00,
								   0xA0, 0x00,
								   0x90, 0x00,
								   0x88, 0x00,
								   0x84, 0x00,
								   0x82, 0x00,
								   0x81, 0x00,
								   0x80, 0x80,
								   0x80, 0x40,
								   0x83, 0xE0,
								   0x92, 0x00,
								   0xA9, 0x00,
								   0xC9, 0x00,
								   0x84, 0x80,
								   0x04, 0x80,
								   0x03, 0x00 };

// . . 
// . X . 
// . X X .  
// . X X X  . 
// . X X X  X . 
// . X X X  X X . 
// . X X X  X X X .  
// . X X X  X X X X  .  
// . X X X  X X X X  X .
// . X X X  X X . .  . . .
// . X X .  X X .  
// . X .    . X X .
// . .      . X X .  
// .          . X X  .
//            . X X  .
//              . .  

unsigned char cursor_large_mask[32] = { 0xC0, 0x00,
								   0xE0, 0x00,
								   0xF0, 0x00,
								   0xF8, 0x00,
								   0xFC, 0x00,
								   0xFE, 0x00,
								   0xFF, 0x00,
								   0xFF, 0x80,
								   0xFF, 0xC0,
								   0xFF, 0xE0,
								   0xFE, 0x00,
								   0xEF, 0x00,
								   0xCF, 0x00,
								   0x87, 0x80,
								   0x07, 0x80,
								   0x03, 0x00 };


extern const unsigned char font[128][8];

unsigned char cursor_buffer[48];

void draw_pixel(uint x, uint y) {
	unsigned char *pixel = (char*)(VGA_ADDRESS + 80*y + x/8);
	*pixel &= ~(128 >> (x % 8));
}

// Draw a character but make sure it stays inside the frame
void draw_font_inside_frame(unsigned char c, uint x, uint y, uint left_x, uint right_x, uint top_y, uint bottom_y) {
	// The font is outside of the frame, do nothing
	if (left_x >= x+8 ||
		right_x < x ||
		top_y >= y+8 ||
		bottom_y < y) return;

	unsigned char *pixel = (char*)(VGA_ADDRESS + 80*y + x/8);
	uint offset_left = x % 8;
	uint offset_right = 8 - offset_left;
	uint left_margin=0, right_margin=0, top_margin=0, bottom_margin=8;
	u8int mask_left, mask_left_bis, mask_right;

	// The left part needs to be left out
	if (left_x > x) left_margin = (left_x - x);
	if (right_x < x+8) right_margin = (x+7 - right_x);
/*
	debug_i("x: ", x);
	debug_i("left_x: ", left_x);
	debug_i("right_x: ", right_x);
	debug_i("Left margin: ", left_margin);
	debug_i("Right margin: ", right_margin);
*/
	if (left_margin > 8-offset_left) {
		mask_left = ~(0xFF >> (offset_left + 8));
		mask_left_bis = 0xFF >> (offset_left + 8);
		mask_right = ~( (0xFF << offset_right) & (0xFF >> (left_margin - 8+offset_left)) );
//		debug("Scenario #1");
	} else if (right_margin > 8-offset_right) {
		mask_left = ~( (0xFF >> offset_left) & (0xFF << (right_margin - 8+offset_right)) );
		mask_left_bis = (0xFF >> offset_left) & (0xFF << (right_margin - 8+offset_right));
		mask_right = ~(0xFF << (offset_right + 8));
//		debug("Scenario #2");
	} else {
		mask_left = ~(0xFF >> offset_left + left_margin);
		mask_left_bis = 0xFF >> offset_left + left_margin;
		mask_right = ~(0xFF << offset_right + right_margin);
//		debug("Scenario #3");
	}

	top_margin = umax(y, top_y) - y;
	bottom_margin = umin(y+7, bottom_y) - y;
	uint c_idx = c;
	for (int i=top_margin; i<=bottom_margin; i++) {
		*pixel &= mask_left;
		*pixel++ |= (~font[c_idx][i] >> offset_left) & mask_left_bis;
		*pixel &= mask_right;
		*pixel++ |= (~font[c_idx][i] << offset_right);
		pixel += 78;
	}
}

void draw_string_inside_frame(const unsigned char *str, int x, int y, uint left_x, uint right_x, uint top_y, uint bottom_y) {
	int len = strlen(str);

	for (int i=0; i<len; i++) {
		draw_font_inside_frame(str[i], x, y, left_x, right_x, top_y, bottom_y);
		x += 8;
	}
}

void draw_font(unsigned char c, uint x, uint y) {
	unsigned char *pixel = (char*)(VGA_ADDRESS + 80*y + x/8);
	uint offset1 = x % 8;
	uint offset2 = 8 - offset1;
	u8int mask1 = ~(0xFF >> offset1);
	u8int mask1bis = 0xFF >> offset1;
	u8int mask2 = ~(0xFF << offset2);
	uint c_idx = c;
	for (int i=0; i<8; i++) {
		*pixel &= mask1;
		*pixel++ |= (~font[c_idx][i] >> offset1) & mask1bis;
		*pixel &= mask2;
		*pixel++ |= (~font[c_idx][i] << offset2);
		pixel += 78;
	}
}

void draw_cursor(uint x, uint y) {
	unsigned char *pixel = (char*)(VGA_ADDRESS + 80*y + x/8);
	uint offset1 = x % 8;
	uint offset2 = 8 - offset1;
	unsigned char *new_pixel;

	uint draw_second_byte = 1;
	uint draw_third_byte = 1;

	if (x >= 640 - 8) draw_second_byte = 0;
	if (x >= 640 - 16) draw_third_byte = 0;

	int bottom = 32;
	if (y >= 480 - 16) bottom = 32 - (y - 480 + 16) * 2;

	for (int i=0; i<bottom; i+=2) {
		new_pixel = pixel + 80;
		*pixel &= ~(cursor_large_mask[i] >> offset1);
		*pixel++ |= cursor_large[i] >> offset1;

		if (draw_second_byte) {
	  		*pixel &= ~(cursor_large_mask[i] << offset2);
			*pixel &= ~(cursor_large_mask[i+1] >> offset1);
			*pixel |= cursor_large[i] << offset2;
			*pixel++ |= cursor_large[i+1] >> offset1;
 
 			if (draw_third_byte) {
  				*pixel &= ~(cursor_large_mask[i+1] << offset2);
				*pixel |= cursor_large[i+1] << offset2;
			}
		}

		pixel = new_pixel;
	}
}

void draw_cursor_buffer(uint x, uint y) {
	unsigned char *pixel = (char*)(VGA_ADDRESS + 80*y + x/8);
	uint offset1 = x % 8;
	uint offset2 = 8 - offset1;

	for (int i=0; i<16; i++) {
		*pixel++ = cursor_buffer[i];
		*pixel++ = cursor_buffer[i+16];
		*pixel++ = cursor_buffer[i+32];
		pixel += 77;
	}
}

void save_cursor_buffer(uint x, uint y) {
	unsigned char *pixel = (char*)(VGA_ADDRESS + 80*y + x/8);
	uint offset1 = x % 8;
	uint offset2 = 8 - offset1;

	for (int i=0; i<16; i++) {
		cursor_buffer[i] = *pixel++;
		cursor_buffer[i+16] = *pixel++;
		cursor_buffer[i+32] = *pixel++;
		pixel += 77;
	}
}

void draw_background(int left_x, int right_x, int top_y, int bottom_y) {
	int i, j;

	// We don't want to go pixel by pixel, it would be too slow
	// So let's map this 32 pixels at a time as we're running 32-bit code
	uint *pixels = (uint*)(VGA_ADDRESS) + 20*top_y + left_x/32;
	char *pixel = (char*)pixels;
	uint *new_pixels;

	// The first thing is to determine the "mask" for the first 32-pixels block
	// Because the block is 32-pixels aligned, it is likely overlapping the content
	// of the window (which we want to fill) and outside of the
	uint start_x = left_x / 32;
	uint start_offset = left_x % 32;
	if (start_offset == 0) start_x--;

//	draw_ptr((void*)start_x, 0, 0);
//	draw_ptr((void*)start_offset, 0, 8);
	
	u8int mask1[4] = { 0x00, 0x00, 0x00, 0x00 };
	u8int bg1[2][4] = { { 0x00, 0x00, 0x00, 0x00 }, { 0x00, 0x00, 0x00, 0x00 } };
	uint *mask1b = (uint*)&mask1;
	uint *bg1b[2] = { (uint*)&bg1[0], (uint*)&bg1[1] };

	for (i=0; i<4; i++) {
		if (start_offset < 8) {
			mask1[i] = 0xFF >> start_offset;
			bg1[0][i] = 0xAA >> start_offset;
			bg1[1][i] = 0x55 >> start_offset;
			for (int j=i+1; j<4; j++) { mask1[j] = 0xFF; bg1[0][j] = 0xAA; bg1[1][j] = 0x55; }
			break;
		}
		start_offset-=8;
	}
	start_offset = left_x % 32;

	uint end_x = right_x / 32 + 1;
	uint end_offset = right_x % 32;

//	draw_ptr((void*)end_x, 0, 16);
//	draw_ptr((void*)end_offset, 0, 24);

	u8int mask2[4] = { 0xFF, 0xFF, 0xFF, 0x00 };
	u8int bg2[2][4] = { { 0xAA, 0xAA, 0xAA, 0x00 }, { 0x55, 0x55, 0x55, 0x00 } };
	uint *mask2b = (uint*)&mask2;
	uint *bg2b[2] = { (uint*)&bg2[0], (uint*)&bg2[1] };

	for (i=0; i<4; i++) {
		if (end_offset < 8) {
			mask2[i] = 0xFF << (7 - end_offset);
			bg2[0][i] = 0xAA << (7 - end_offset);
			bg2[1][i] = 0x55 << (7 - end_offset);
			for (int j=i+1; j<4; j++) { mask2[j] = 0x00; bg2[0][j] = 0x00; bg2[1][j] = 0x00; }
			break;
		}
		end_offset-=8;
	}
	end_offset = right_x % 32;

	uint bg_value = 0xAAAAAAAA;
	uint bg_idx = 1;
	if (left_x + top_y % 2 == 0) {
		bg_value = ~bg_value;
		bg_idx = 0;
	}

	// For each line of the window
	for (j=top_y; j<=bottom_y; j++) {
		new_pixels = pixels + 20;

		// Fills the first 32-block of the window
		if (start_offset > 0) {
			*pixels |= *mask1b;
			*pixels++ ^= *bg1b[bg_idx];
		}

		// Fills the content, 32 pixels at a time
		for (i=start_x+1; i<end_x-1; i++) {
			*pixels++ = bg_value;
		}
		
		// Fills the last 32-block of the window
		if (end_offset > 0) {
			*pixels |= *mask2b;
			*pixels ^= *bg2b[bg_idx];
		}

		// Get ready for the next line
		pixels = new_pixels;
		bg_value = ~bg_value;
		bg_idx = 1 - bg_idx;		
	}

//	draw_frame(left_x, right_x, top_y, bottom_y);
}

void draw_frame(int left_x, int right_x, int top_y, int bottom_y) {
	int i, safe_left_x = left_x, safe_right_x = right_x, safe_top_y = top_y, safe_bottom_y = bottom_y;
	if (safe_left_x < 0) safe_left_x = 0;
	if (safe_right_x >= 640) safe_right_x = 639;
	if (safe_top_y < 0) safe_top_y = 0;
	if (safe_bottom_y >= 480) safe_bottom_y = 479;

	// Draws the top and bottom lines of the window
	for (i=safe_left_x; i<safe_right_x; i++) {
		if (top_y >= 0) draw_pixel(i, top_y);
		if (bottom_y <480) draw_pixel(i, bottom_y);
	}

	// Draws the left and right lines of the window
	for (i=safe_top_y; i<=safe_bottom_y; i++) {
		if (left_x >= 0) draw_pixel(left_x, i);
		if (right_x < 640) draw_pixel(right_x, i);
	}
}

void draw_string(const unsigned char *str, int x, int y) {
	int len = strlen(str);

	for (int i=0; i<len; i++) {
		draw_font(str[i], x, y);
		x += 8;
	}
}

void draw_string_n(const unsigned char *str, int x, int y, int len) {
	for (int i=0; i<len; i++) {
		draw_font(str[i], x, y);
		x += 8;
	}
}

// Fills the box with white
void draw_box(uint left_x, uint right_x, uint top_y, uint bottom_y) {
	int i, j;

	// We don't want to go pixel by pixel, it would be too slow
	// So let's map this 32 pixels at a time as we're running 32-bit code
	uint *pixels = (uint*)(VGA_ADDRESS) + 20*top_y + left_x/32;
	char *pixel = (char*)pixels;
	uint *new_pixels;

	// The first thing is to determine the "mask" for the first 32-pixels block
	// Because the block is 32-pixels aligned, it is likely overlapping the content
	// of the window (which we want to fill) and outside of the
	uint start_x = left_x / 32;
	uint start_offset = left_x % 32;
	if (start_offset == 0) start_x--;

//	draw_ptr((void*)start_x, 0, 0);
//	draw_ptr((void*)start_offset, 0, 8);
	
	u8int mask1[4] = { 0x00, 0x00, 0x00, 0x00 };
	uint *mask1b = (uint*)&mask1;

	for (i=0; i<4; i++) {
		if (start_offset < 8) {
			mask1[i] = 0xFF >> start_offset;
			for (int j=i+1; j<4; j++) mask1[j] = 0xFF;
			break;
		}
		start_offset-=8;
	}
	start_offset = left_x % 32;

	uint end_x = right_x / 32 + 1;
	uint end_offset = right_x % 32;

//	draw_ptr((void*)end_x, 0, 16);
//	draw_ptr((void*)end_offset, 0, 24);

	u8int mask2[4] = { 0xFF, 0xFF, 0xFF, 0xFF };
	uint *mask2b = (uint*)&mask2;
	
	for (i=0; i<4; i++) {
		if (end_offset < 8) {
			mask2[i] = 0xFF << (7 - end_offset);
			for (int j=i+1; j<4; j++) mask2[j] = 0x00;
			break;
		}
		end_offset-=8;
	}
	end_offset = right_x % 32;

	// For each line of the window
	for (j=top_y; j<=bottom_y; j++) {
		new_pixels = pixels + 20;

		// Fills the first 32-block of the window
		if (start_offset > 0) *pixels++ |= *mask1b;

		// Fills the content, 32 pixels at a time
		for (i=start_x+1; i<end_x-1; i++) {
			*pixels++ = 0xFFFFFFFF;
		}

		// Fills the last 32-block of the window
		if (end_offset > 0) *pixels |= *mask2b;

		pixels = new_pixels;
	}
}

// This version of copy_box() is incomplete. Right now it copies the sides outside the
// box. In the way it's currently used it doesn't show, but that needs to be fixed
// nonetheless
void copy_box(uint left_x, uint right_x, uint top_y, uint bottom_y, uint nb_pixels_up) {
	int i, j;

	// We don't want to go pixel by pixel, it would be too slow
	// So let's map this 32 pixels at a time as we're running 32-bit code
	uint *pixels = (uint*)(VGA_ADDRESS) + 20*top_y + left_x / 32;
	char *pixel = (char*)pixels;
	uint *new_pixels;

	// The first thing is to determine the "mask" for the first 32-pixels block
	// Because the block is 32-pixels aligned, it is likely overlapping the content
	// of the window (which we want to fill) and outside of the
	uint start_x = left_x / 32;
	uint start_offset = left_x % 32;
	u8int mask1[4] = { 0x00, 0x00, 0x00, 0x00 };
	uint *mask1b = (uint*)&mask1;

	for (i=0; i<4; i++) {
		if (start_offset < 8) {
			mask1[i] = 0xFF >> start_offset;
			for (int j=i+1; j<4; j++) mask1[j] = 0xFF;
			break;
		}
		start_offset-=8;
	}

	uint end_x = right_x / 32 + 1;
	uint end_offset = right_x % 32;
	u8int mask2[4] = { 0xFF, 0xFF, 0xFF, 0xFF };
	uint *mask2b = (uint*)&mask2;
	
	for (i=0; i<4; i++) {
		if (end_offset < 8) {
			mask2[i] = 0xFF << (7 - end_offset);
			for (int j=i+1; j<4; j++) mask2[j] = 0x00;
			break;
		}
		end_offset-=8;
	}

	// For each line of the window
	for (j=top_y; j<=bottom_y; j++) {
		new_pixels = pixels + 20;

		// Fills the first 32-block of the window
//		*pixels |= *mask1b;
		*pixels = *(pixels + 20 * nb_pixels_up);
		pixels++;

		// Fills the content, 32 pixels at a time
		for (i=start_x+1; i<end_x-1; i++) {
			*pixels = *(pixels + 20 * nb_pixels_up);
			pixels++;
		}

		// Fills the last 32-block of the window
//		*pixels |= *mask2b;
		*pixels = *(pixels + 20 * nb_pixels_up);

		pixels = new_pixels;
	}
}

#define PREPARE_FIRST_COPY()                                      \
    do {                                                          \
    if (src_len >= (CHAR_BIT - dst_offset_modulo)) {              \
        *dst     &= reverse_mask[dst_offset_modulo];              \
        src_len -= CHAR_BIT - dst_offset_modulo;                  \
    } else {                                                      \
        *dst     &= reverse_mask[dst_offset_modulo]               \
              | reverse_mask_xor[dst_offset_modulo + src_len];    \
         c       &= reverse_mask[dst_offset_modulo + src_len];    \
        src_len = 0;                                              \
    } } while (0)

#define CHAR_BIT 8

void
bitarray_copy(const unsigned char *src_org, int src_offset, int src_len,
                    unsigned char *dst_org, int dst_offset)
{
    static const unsigned char mask[] =
        { 0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff };
    static const unsigned char reverse_mask[] =
        { 0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff };
    static const unsigned char reverse_mask_xor[] =
        { 0xff, 0x7f, 0x3f, 0x1f, 0x0f, 0x07, 0x03, 0x01, 0x00 };

    if (src_len) {
        const unsigned char *src;
              unsigned char *dst;
        int                  src_offset_modulo,
                             dst_offset_modulo;

        src = src_org + (src_offset / CHAR_BIT);
        dst = dst_org + (dst_offset / CHAR_BIT);

        src_offset_modulo = src_offset % CHAR_BIT;
        dst_offset_modulo = dst_offset % CHAR_BIT;

        if (src_offset_modulo == dst_offset_modulo) {
            int              byte_len;
            int              src_len_modulo;
            if (src_offset_modulo) {
                unsigned char   c;

                c = reverse_mask_xor[dst_offset_modulo]     & *src++;

                PREPARE_FIRST_COPY();
                *dst++ |= c;
            }

            byte_len = src_len / CHAR_BIT;
            src_len_modulo = src_len % CHAR_BIT;

            if (byte_len) {
                memcpy(dst, src, byte_len);
                src += byte_len;
                dst += byte_len;
            }
            if (src_len_modulo) {
                *dst     &= reverse_mask_xor[src_len_modulo];
                *dst |= reverse_mask[src_len_modulo]     & *src;
            }
        } else {
            int             bit_diff_ls,
                            bit_diff_rs;
            int             byte_len;
            int             src_len_modulo;
            unsigned char   c;
            /*
             * Begin: Line things up on destination. 
             */
            if (src_offset_modulo > dst_offset_modulo) {
                bit_diff_ls = src_offset_modulo - dst_offset_modulo;
                bit_diff_rs = CHAR_BIT - bit_diff_ls;

                c = *src++ << bit_diff_ls;
                c |= *src >> bit_diff_rs;
                c     &= reverse_mask_xor[dst_offset_modulo];
            } else {
                bit_diff_rs = dst_offset_modulo - src_offset_modulo;
                bit_diff_ls = CHAR_BIT - bit_diff_rs;

                c = *src >> bit_diff_rs     &
                    reverse_mask_xor[dst_offset_modulo];
            }
            PREPARE_FIRST_COPY();
            *dst++ |= c;

            /*
             * Middle: copy with only shifting the source. 
             */
            byte_len = src_len / CHAR_BIT;

            while (--byte_len >= 0) {
                c = *src++ << bit_diff_ls;
                c |= *src >> bit_diff_rs;
                *dst++ = c;
            }

            /*
             * End: copy the remaing bits; 
             */
            src_len_modulo = src_len % CHAR_BIT;
            if (src_len_modulo) {
                c = *src++ << bit_diff_ls;
                c |= *src >> bit_diff_rs;
                c     &= reverse_mask[src_len_modulo];

                *dst     &= reverse_mask_xor[src_len_modulo];
                *dst |= c;
            }
        }
    }
}

void init_vga() {
	char *address = (char*)VGA_ADDRESS;

	for (int i=0; i<480; i+=2) {
    	memset(address, 0xAA, 640/8);
    	address += 640/8;
    	memset(address, 0x55, 640/8);
    	address += 640/8;
    }

	gui_save_mouse_buffer();
}
