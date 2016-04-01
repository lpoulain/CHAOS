#ifndef __DHCP_H
#define __DHCP_H

#include "libc.h"

uint DNS_query(char *hostname);
void DNS_receive_packet(uint8* buffer, uint16 size);

#endif
