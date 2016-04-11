#include "libc.h"
#include "ipv4.h"
#include "tcp.h"
#include "kheap.h"
#include "debug.h"

#define TCP_HEADER_SIZE		20

#define TCP_FLAGS_FIN		1
#define TCP_FLAGS_SYN		2
#define TCP_FLAGS_RESET		4
#define TCP_FLAGS_PUSH		8
#define TCP_FLAGS_ACK		16
#define TCP_FLAGS_URGENT	32
#define TCP_FLAGS_ECHO		64
#define TCP_FLAGS_CWR		128

uint8 TCP_options[12] = { 0x01, 0x01, 0x08, 0x0A, 0x37, 0x8D, 0xE5, 0xDB, 0xA8, 0xF4, 0x91, 0xC6 };

typedef struct __attribute__((packed)) {
	uint16 sport;
	uint16 dport;
	uint sequence_nb;
	uint ack_nb;
	uint8 header_size;
	uint8 flags;
	uint16 win_size_value;
	uint16 checksum;
	uint16 urgent;
} TCPHeader;

TCPConnection connection;

uint16 TCP_checksum(TCPHeader *header, uint16 tcp_packet_size) {
	uint sum = 0;
	uint16 *body = (uint16*)((uint8*)header);
//	uint16 body_size = switch_endian16(header->header_size + payload_size);

	for (int i=0; i<tcp_packet_size/2; i++) {
		sum += switch_endian16(*body++);
	}

	if (tcp_packet_size % 2 == 1) {
		uint16 tmp = (*body) & 0xFF;
		sum += switch_endian16(tmp);
	}

	// Because the TCP checksum is using the source and dest
	// IP address for the checksum, we're looking at the IP header
	IPv4Header *header_ip = (IPv4Header*)((uint8*)header - IPV4_HEADER_SIZE);

	sum += switch_endian16(header_ip->ip_src & 0xFFFF);
	sum += switch_endian16(header_ip->ip_src >> 16);
	sum += switch_endian16(header_ip->ip_dst & 0xFFFF);
	sum += switch_endian16(header_ip->ip_dst >> 16);
	sum += tcp_packet_size;
	sum += IPV4_PROTOCOL_TCP;

	while (sum >> 16) {
		sum = (sum & 0xFFFF) + (sum >> 16);
	}

	return ~switch_endian16((uint16)sum);
}

uint8 *TCP_send_packet(uint ipv4, uint16 sport, uint16 dport,
					   uint sequence_nb, uint ack_nb,
					   uint8 flags,
					   uint8 *options, uint16 options_length,
					   uint8* payload, uint16 payload_size) {
	// Get an IPv4 packet
	uint16 size = payload_size + TCP_HEADER_SIZE + options_length;
	uint16 offset;
	uint8 *buffer = IPv4_create_packet(IPV4_PROTOCOL_TCP, ipv4, size, &offset);

	// Append the TCP header
	TCPHeader *header = (TCPHeader*)(buffer + offset);
	header->sport = switch_endian16(sport);
	header->dport = switch_endian16(dport);
	header->sequence_nb = switch_endian32(sequence_nb);
	header->ack_nb = switch_endian32(ack_nb);
	header->header_size = ((TCP_HEADER_SIZE + options_length) / 4) << 4;
	header->flags = flags;
	header->win_size_value = 0xFFFF;
	header->urgent = 0;
	header->checksum = 0;

	if (options_length > 0) {
		for (int i=0; i<options_length; i++) {
			buffer[offset + TCP_HEADER_SIZE + i] = options[i];
		}
	}

	if (payload_size > 0)
		strncpy(buffer + offset + TCP_HEADER_SIZE + options_length, payload, payload_size);

/*	uint8 options[12] = { 0x01, 0x01, 0x08, 0x0A, 0x37, 0x8D, 0xE5, 0xDB, 0xA8, 0xF4, 0x91, 0xC6 };
	for (int i=0; i<12; i++) {
		buffer[offset + TCP_HEADER_SIZE + i] = options[i];
	}
*/
	header->checksum = TCP_checksum(header, TCP_HEADER_SIZE + options_length + payload_size);

	IPv4_send_packet(buffer, size + offset, offset);
}

void TCP_end_connection(uint ipv4) {

}

void TCP_receive_packet(uint ipv4_from, uint8 *buffer, uint16 size) {
	TCPHeader *header = (TCPHeader*)buffer;
	if (is_debug()) printf("[TCP %d]", switch_endian16(header->dport));
	uint16 flags, header_size = (header->header_size >> 4) * 4;
	int payload_size;
	uint16 port = switch_endian16(header->dport);

	if (port == connection.sport) {
		flags = header->flags;
		if (is_debug()) printf("[HTTP] (%x)\n", flags);
		switch(connection.status) {

			// TCP handshake
			case TCP_STATUS_HANDSHAKE_SYN:
				if (flags & TCP_FLAGS_SYN) {
					TCP_send_packet(ipv4_from, connection.sport, connection.dport,
									switch_endian32(header->ack_nb), switch_endian32(header->sequence_nb) + 1,
									TCP_FLAGS_ACK,
									TCP_options, 12,
									0, 0);
					connection.status = TCP_STATUS_HANDSHAKE_ACK;

					TCP_send_packet(ipv4_from, connection.sport, connection.dport,
									switch_endian32(header->ack_nb), switch_endian32(header->sequence_nb) + 1,
									TCP_FLAGS_PUSH | TCP_FLAGS_ACK,
									TCP_options, 12,
									connection.payload, connection.payload_size);

					connection.status = TCP_STATUS_TRANSFER_PUSH;
				}
				return;

			case TCP_STATUS_TRANSFER_PUSH:
				if (flags & TCP_FLAGS_FIN) {
					TCP_send_packet(ipv4_from, connection.sport, connection.dport,
									switch_endian32(header->ack_nb), switch_endian32(header->sequence_nb) + 1,
									TCP_FLAGS_ACK,
									TCP_options, 12,
									0, 0);
					return;
				}

				if (size > header_size) {
					payload_size = size - header_size;
					connection.data = kmalloc(payload_size + 1);
					strncpy(connection.data, buffer + header_size, payload_size);
					connection.data[payload_size] = 0;
					connection.data_size = payload_size;
					connection.status = TCP_STATUS_FIN;
				}
				TCP_send_packet(ipv4_from, connection.sport, connection.dport,
								switch_endian32(header->ack_nb), switch_endian32(header->sequence_nb) + 1,
								TCP_FLAGS_ACK,
								TCP_options, 12,
								0, 0);
				return;
		}
	}
	else {
			if (is_debug()) printf("[%d]\n", switch_endian16(header->dport));
	}
}

TCPConnection *TCP_start_connection(uint ipv4, uint16 dport, uint8 *payload, uint16 payload_size) {

	connection.status = TCP_STATUS_HANDSHAKE_SYN;
	connection.sport = 10000 + rand();
	connection.dport = dport;
	connection.payload = payload;
	connection.payload_size = payload_size;
	connection.data = 0;

	TCP_send_packet(ipv4, connection.sport, connection.dport,
					10, 0,
					TCP_FLAGS_SYN,
					TCP_options, 12,
					0, 0);

	return &connection;
}
