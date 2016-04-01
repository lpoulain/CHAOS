#include "libc.h"
#include "ipv4.h"
#include "icmp.h"

#define ICMP_HEADER_SIZE 		16

typedef struct {
	uint8 type;
	uint8 code;
	uint16 checksum;
	uint16 id;
	uint16 seq;
	uint8 timestamp[8];
	uint8 data[48];
} ICMPHeader;

uint8 ping_data[48] = {
	0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
	0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
	0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37
};

uint16 ICMP_checksum(uint8* buffer, uint16 size) {
	uint sum = 0;
	uint16 *body = (uint16*)((uint8*)buffer);

	for (int i=0; i<size/2; i++) {
		sum += switch_endian16(*body++);
	}

	while (sum >> 16) {
		sum = (sum & 0xFFFF) + (sum >> 16);
	}	

	return ~switch_endian16((uint16)sum);
}

typedef struct {
	uint16 txn_id;
	uint8 status;
} ICMPRegistration;

ICMPRegistration registration[2];

void ICMP_register_reply(uint ps_id) {
	registration[ps_id].txn_id = (uint16)ps_id | 0xF4A0;
	registration[ps_id].status = ICMP_TYPE_ECHO_REQUEST;
}

void ICMP_unregister_reply(uint ps_id) {
	registration[ps_id].txn_id = 0;
	registration[ps_id].status = 0;
}

uint8 ICMP_check_response(uint ps_id) {
	return registration[ps_id].status;
}

void ICMP_send_packet(uint ipv4, uint ps_id) {
//	printf_win(win, "MAC address: %X:%X:%X:%X:%X:%X\n", E1000_adapter.MAC[0], E1000_adapter.MAC[1], E1000_adapter.MAC[2], E1000_adapter.MAC[3], E1000_adapter.MAC[4], E1000_adapter.MAC[5]);
	uint16 offset;

	uint8* buffer = IPv4_create_packet(IPV4_PROTOCOL_ICMP, ipv4, 64, &offset);

	// Append the ICMP header
	ICMPHeader *header = (ICMPHeader*)(buffer + offset);

	header->type = ICMP_TYPE_ECHO_REQUEST;
	header->code = 0;
	header->checksum = 0;
	header->id = (uint16)ps_id | 0xF4A0;
	header->seq = 0;
	header->timestamp[0] = 0x56;
	header->timestamp[1] = 0xFB;
	header->timestamp[2] = 0x3F;
	header->timestamp[3] = 0x21;
	header->timestamp[4] = 0x00;
	header->timestamp[5] = 0x07;
	header->timestamp[6] = 0x1C;
	header->timestamp[7] = 0xD6;

	strncpy((char*)&header->data, ping_data, 48);
	header->checksum = ICMP_checksum((uint8*)header, 64);

	IPv4_send_packet(buffer, 64 + offset, offset);
}

void ICMP_receive_packet(uint ipv4, uint8* buffer_ping, uint16 size) {

	ICMPHeader *header_ping = (ICMPHeader*)buffer_ping;
	printf("Pong from %x, code=%d\n", ipv4, header_ping->type);

	if (header_ping->type == ICMP_TYPE_ECHO_REPLY || header_ping->type == ICMP_TYPE_ECHO_UNREACHABLE) {
		for (int i=0; i<2; i++) {
			if (registration[i].txn_id == header_ping->id) {
				registration[i].status = header_ping->id;
				return;
			}
		}
		return;
	}

	if (header_ping->type == ICMP_TYPE_ECHO_REQUEST) {
		uint16 offset, ping_msg_size = size - ICMP_HEADER_SIZE;

		uint8* buffer_pong = IPv4_create_packet(IPV4_PROTOCOL_ICMP, ipv4, size, &offset);

		// Append the ICMP header
		ICMPHeader *header_pong = (ICMPHeader*)(buffer_pong + offset);

		header_pong->type = ICMP_TYPE_ECHO_REPLY;
		header_pong->code = 0;
		header_pong->checksum = 0;
		header_pong->id = header_ping->id;
		header_pong->seq = header_ping->seq;
		for (int i=0; i<8; i++)
			header_pong->timestamp[i] = header_ping->timestamp[i];

		strncpy((char*)&header_pong->data, buffer_ping + ICMP_HEADER_SIZE, ping_msg_size);

		header_pong->checksum = ICMP_checksum((uint8*)header_pong, size);

	//	dump_mem(buffer_pong, size, 14);
	//	for (;;);

		IPv4_send_packet(buffer_pong, size + offset, offset);
		return;
	}
}
