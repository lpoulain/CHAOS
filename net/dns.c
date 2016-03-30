#include "libc.h"
#include "network.h"
#include "udp.h"

typedef struct __attribute__((packed)) {
	uint16 txn_id;
	uint16 flags;
	uint16 questions;
	uint16 answer_rr;
	uint16 authority_rr;
	uint16 addnl_rr;
} DNSHeader;

typedef struct {
	char hostname[256];
	uint ipv4;
} DNSEntry;

DNSEntry DNS_table[100];
uint nb_DNS_entries;

int DNS_get_entry(char *hostname) {
	for (int i=0; i<nb_DNS_entries; i++) {
		if (!strcmp(hostname, DNS_table[i].hostname)) return i;
	}

	return -1;
}

void DNS_add_entry(char *hostname, uint ipv4) {
	int idx = DNS_get_entry(hostname);

	if (idx >= 0) {
		DNS_table[idx].ipv4 = ipv4;
		return;
	}

	strcpy(DNS_table[nb_DNS_entries].hostname, hostname);
	DNS_table[nb_DNS_entries].ipv4 = ipv4;
	nb_DNS_entries++;
}

void DNS_send_packet(char *hostname) {
//	printf_win(win, "MAC address: %X:%X:%X:%X:%X:%X\n", E1000_adapter.MAC[0], E1000_adapter.MAC[1], E1000_adapter.MAC[2], E1000_adapter.MAC[3], E1000_adapter.MAC[4], E1000_adapter.MAC[5]);
	uint16 offset;
	uint len = strlen(hostname);

	// We want to send an UDP packet to IP address 255.255.255.255 (broadcast)
	// source port=68, dest port=67 whose message is 300 bytes long (excluding headers)
	uint8* buffer = UDP_create_packet(network_get_DNS(), 0xCECE, UDP_PORT_DNS, 18 + len, &offset);

	// Append the HDCP header
	DNSHeader *header = (DNSHeader*)(buffer + offset);
	header->txn_id = 0x9145;
	header->flags = 0x0001;
	header->questions = switch_endian16(1);
	header->answer_rr = 0;
	header->authority_rr = 0;
	header->addnl_rr = 0;

	int len_offset = offset + 12;
	int write_offset = offset + 13;
	for (int i=0; i<len; i++) {
		if (hostname[i] == '.') {
			buffer[len_offset] = write_offset - len_offset - 1;
			len_offset = write_offset;
		} else {
			buffer[write_offset] = hostname[i];
		}
		write_offset++;
	}
	buffer[len_offset] = write_offset - len_offset - 1;

	buffer[offset + 13 + len + 2] = 0x01;
	buffer[offset + 13 + len + 4] = 0x01;

	UDP_send_packet(buffer, offset + 18 + len, offset);
}

void DNS_receive_packet(uint8* buffer, uint16 size) {
	DNSHeader *header = (DNSHeader*)buffer;

	unsigned char *hostname = buffer + 13;
	uint *ipv4 = (uint*)(buffer + size - 4);

	DNS_add_entry(hostname, *ipv4);
}

uint DNS_query(char *hostname) {
	int idx = DNS_get_entry(hostname);

	if (idx >= 0)
		return DNS_table[idx].ipv4;

	DNS_send_packet(hostname);

	for (int i=0; i<200000000; i++) {
 		idx = DNS_get_entry(hostname);
		if (idx >= 0)
			return DNS_table[idx].ipv4;
	}

	return 0;
}
