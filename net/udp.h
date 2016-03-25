#ifndef __UDP_H
#define __UDP_H

#include "libc.h"

#define UDP_PORT_DHCP		68

uint8 *UDP_create_packet(uint ipv4, uint16 sport, uint16 dport, uint16 size, uint16 *offset);
void UDP_send_packet(uint8 *buffer, uint16 size, uint16 message_offset);
void UDP_receive_packet(uint8 *buffer);

#endif
