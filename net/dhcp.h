#ifndef __DHCP_H
#define __DHCP_H

#include "libc.h"

void DHCP_send_packet();
void DHCP_receive_packet(uint8* buffer, uint16 size);

#endif
