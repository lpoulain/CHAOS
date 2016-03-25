#include "libc.h"
#include "network.h"
#include "ethernet.h"
#include "ipv4.h"
#include "udp.h"

#define IPV4_UDP	0x11
#define IPV4_TCP 	0x06

uint16 IPv4_checksum(IPv4Header *header) {
	uint sum = 0;
	uint16 *elt = (uint16*)header;
	for (int i=0; i<IPV4_HEADER_SIZE/2; i++) {
		sum += switch_endian16(*elt++);
	}

	while (sum >> 16) {
		sum = (sum & 0xFFFF) + (sum >> 16);
	}

	return ~switch_endian16((uint16)sum);
}

uint8 *IPv4_create_packet(uint ipv4, uint16 size, uint16 *offset) {
	// Get an Ethernet packet
	size += IPV4_HEADER_SIZE;
	uint8 *buffer = ethernet_create_packet(ETHERNET_IPV4, ipv4, size, offset);

	// Append the IPv4 header
	IPv4Header *header = (IPv4Header*)(buffer + *offset);
	header->header = 0x45;
	header->diff_svc_fld = 0x10;
	header->length = switch_endian16(size);
	header->id = 0;
	header->flags = 0x00;
	header->fragment_offset = 0x0000;
	header->time_to_live = 0x80;
	header->protocol = IPV4_UDP;
	header->ip_src = 0;
	header->ip_dst = ipv4;
	header->checksum = IPv4_checksum(header);

	*offset += IPV4_HEADER_SIZE;
	return buffer;
}

void IPv4_send_packet(uint8 *buffer, uint16 size, uint16 message_offset) {
	ethernet_send_packet(buffer, size);
}

void IPv4_receive_packet(uint8 *buffer) {
	IPv4Header *header = (IPv4Header*)buffer;
	
	switch(header->protocol) {
		case IPV4_UDP:
			UDP_receive_packet(buffer + IPV4_HEADER_SIZE);
			break;
		case IPV4_TCP:
			break;
	}
}
