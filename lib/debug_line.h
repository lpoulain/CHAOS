#ifndef __DEBUG_LINE_H
#define __DEBUG_LINE_H

#include "libc.h"
#include "debug.h"

int debug_line_find_address(void *ptr, StackFrame *frame);

#endif
