#ifndef __DNS_H
#define __DNS_H

#include "libc.h"
#include "display.h"

uint DNS_query(char *hostname);
void DNS_receive_packet(uint8* buffer, uint16 size);
void DNS_print_table(Window *win);

#endif
