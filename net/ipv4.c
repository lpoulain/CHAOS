#include "libc.h"
#include "network.h"
#include "ethernet.h"
#include "ipv4.h"
#include "tcp.h"
#include "udp.h"
#include "icmp.h"
#include "debug.h"

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

uint16 id = 0x2424;

uint8 *IPv4_create_packet(uint8 protocol, uint ipv4, uint16 size, uint16 *offset) {
	// Get an Ethernet packet
	size += IPV4_HEADER_SIZE;
	uint8 *buffer = ethernet_create_packet(ETHERNET_IPV4, ipv4, size, offset);

	// Append the IPv4 header
	IPv4Header *header = (IPv4Header*)(buffer + *offset);
	header->header = 0x45;
	header->diff_svc_fld = 0x00;
	header->length = switch_endian16(size);
	header->id = id++;
	header->flags = 0x00;
	header->fragment_offset = 0x0000;
	header->time_to_live = 0x80;
	header->protocol = protocol;
	header->ip_src = network_get_IPv4();
	header->ip_dst = ipv4;
	header->checksum = IPv4_checksum(header);

	*offset += IPV4_HEADER_SIZE;
	return buffer;
}

void IPv4_send_packet(uint8 *buffer, uint16 size, uint16 message_offset) {
	ethernet_send_packet(buffer, size);
}

void IPv4_receive_packet(uint8 *buffer, uint16 size) {
	IPv4Header *header = (IPv4Header*)buffer;
	uint8* ip = (uint8*)&header->ip_src;
	if (is_debug()) printf("=> [IP %d.%d.%d.%d]", ip[0], ip[1], ip[2], ip[3]);

	switch(header->protocol) {
		case IPV4_PROTOCOL_UDP:
			UDP_receive_packet(buffer + IPV4_HEADER_SIZE);
			break;
		case IPV4_PROTOCOL_TCP:
			TCP_receive_packet(header->ip_src, buffer + IPV4_HEADER_SIZE, size - IPV4_HEADER_SIZE);
			break;
		case IPV4_PROTOCOL_ICMP:
			ICMP_receive_packet(header->ip_src, buffer + IPV4_HEADER_SIZE, switch_endian16(header->length) - IPV4_HEADER_SIZE);
	}
}
