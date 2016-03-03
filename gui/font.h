#include "libc.h"

#define FONT_CHICAGO12PT	0
#define FONT_OLDWIZARD		1

typedef const unsigned char(font_ascii)[128][35];

typedef struct {
	uint8 top;
	uint8 bottom;
	font_ascii *font;
} Typeface;

extern Typeface *typeface;
extern void set_typeface(uint8);
