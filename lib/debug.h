#ifndef __DEBUG_H
#define __DEBUG_H

#include "libc.h"

uint8 switch_debug();
uint8 is_debug();

typedef struct {
	char *function;
	char *path;
	char *filename;
	uint line_number;
} StackFrame;

#endif
