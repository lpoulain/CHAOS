#ifndef __ETHERNET_H
#define __ETHERNET_H

#include "libc.h"

#define ETHERNET_IPV4	0x0008
#define ETHERNET_ARP 	0x0608

uint8 *ethernet_create_packet(uint16 protocol, uint ipv4, uint16 size, uint16 *offset);
void ethernet_send_packet(uint8* buffer, uint16 size);
void ethernet_receive_packet(uint8* buffer);

#endif
