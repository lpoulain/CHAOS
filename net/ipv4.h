#ifndef __IPV4_H
#define __IPV4_H

#include "libc.h"

#define IPV4_HEADER_SIZE	20
#define IPV4_PROTOCOL_ICMP	0x01
#define IPV4_PROTOCOL_TCP	0x06
#define IPV4_PROTOCOL_UDP	0x11

typedef struct __attribute__((packed)) {
	uint8 header;
	uint8 diff_svc_fld;
	uint16 length;
	uint16 id;
	uint8 flags;
	uint8 fragment_offset;
	uint8 time_to_live;
	uint8 protocol;
	uint16 checksum;
	uint ip_src;
	uint ip_dst;
} IPv4Header;

uint8 *IPv4_create_packet(uint8 protocol, uint ipv4, uint16 size, uint16 *offset);
void IPv4_send_packet(uint8 *buffer, uint16 size, uint16 message_offset);
void IPv4_receive_packet(uint8 *buffer, uint16 size);

#endif
