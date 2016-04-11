#include "libc.h"
#include "ipv4.h"
#include "udp.h"
#include "dhcp.h"
#include "dns.h"
#include "debug.h"

#define UDP_HEADER_SIZE		8

typedef struct __attribute__((packed)) {
	uint16 sport;
	uint16 dport;
	uint16 size;
	uint16 checksum;
} UDPHeader;

uint16 UDP_checksum(UDPHeader *header) {
	uint sum = 0;
	uint16 *body = (uint16*)((uint8*)header);
	uint16 body_size = switch_endian16(header->size);

	for (int i=0; i<body_size/2; i++) {
		sum += switch_endian16(*body++);
	}

	if (body_size % 2 == 1) {
		uint16 tmp = (*body) & 0xFF;
		sum += switch_endian16(tmp);
	}
	
	// Because the UDP checksum is using the source and dest
	// IP address for the checksum, we're looking at the IP header
	IPv4Header *header_ip = (IPv4Header*)((uint8*)header - IPV4_HEADER_SIZE);

	sum += switch_endian16(header_ip->ip_src & 0xFFFF);
	sum += switch_endian16((header_ip->ip_src >> 16) & 0xFFFF);
	sum += switch_endian16(header_ip->ip_dst & 0xFFFF);
	sum += switch_endian16((header_ip->ip_dst >> 16) & 0xFFFF);
	sum += switch_endian16(header->size);
	sum += IPV4_PROTOCOL_UDP;

	while (sum >> 16) {
		sum = (sum & 0xFFFF) + (sum >> 16);
	}

	return ~switch_endian16((uint16)sum);
}

uint8 *UDP_create_packet(uint ipv4, uint16 sport, uint16 dport, uint16 size, uint16 *offset) {
	// Get an IPv4 packet
	size += UDP_HEADER_SIZE;
	uint8 *buffer = IPv4_create_packet(IPV4_PROTOCOL_UDP, ipv4, size, offset);

	// Append the UDP header
	UDPHeader *header = (UDPHeader*)(buffer + *offset);
	header->sport = switch_endian16(sport);
	header->dport = switch_endian16(dport);
	header->size = switch_endian16(size);
	header->checksum = 0;

	*offset += UDP_HEADER_SIZE;
	return buffer;
}

void UDP_send_packet(uint8 *buffer, uint16 size, uint16 message_offset) {
	// NEED TO: compute the checksum
	UDPHeader *header = (UDPHeader*)(buffer + message_offset - UDP_HEADER_SIZE);
	header->checksum = UDP_checksum(header);

	IPv4_send_packet(buffer, size, message_offset);
}

void UDP_receive_packet(uint8 *buffer) {
	UDPHeader *header = (UDPHeader*)buffer;
	if (is_debug()) printf("[UDP]");

	switch(switch_endian16(header->dport)) {
		case UDP_PORT_DHCP:
			if (is_debug()) printf("[DHCP]\n");
			DHCP_receive_packet(buffer + UDP_HEADER_SIZE, header->size - UDP_HEADER_SIZE);
			break;
		case UDP_PORT_DNS:
			if (is_debug()) printf("[DNS]\n");
			DNS_receive_packet(buffer + UDP_HEADER_SIZE, header->size - UDP_HEADER_SIZE);
			break;
		default:
			printf("[%d]\n", switch_endian16(header->dport));
	}
}
