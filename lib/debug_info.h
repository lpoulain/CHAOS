#ifndef __DEBUG_INFO_H
#define __DEBUG_INFO_H

#include "debug.h"

int debug_info_find_address(void *ptr, StackFrame *frame);
void debug_info_load();

#endif
