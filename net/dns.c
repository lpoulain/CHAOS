#include "libc.h"
#include "network.h"
#include "udp.h"
#include "display.h"

#define DNS_FLAG_QUERY			0x0001
#define DNS_FLAG_RESPONSE		0x0080

#define DNS_TYPE_HOST_ADDRESS	0x0100
#define DNS_TYPE_ALIAS			0x0500

typedef struct __attribute__((packed)) {
	uint16 txn_id;
	uint16 flags;
	uint16 questions;
	uint16 answer_rr;
	uint16 authority_rr;
	uint16 addnl_rr;
} DNSHeader;

typedef struct __attribute__((packed)) {
	uint16 name;
	uint16 type;
	uint16 class;
	uint time_to_live;
	uint16 data_length;
} DNSAnswer;

typedef struct {
	uint8 *hostname;
	uint8 *alias;
} DNSAlias;

typedef struct {
	char hostname[256];
	uint ipv4;
} DNSEntry;

DNSEntry DNS_table[100];
uint nb_DNS_entries;

void DNS_print_table(Window *win) {
	for (int i=0; i<nb_DNS_entries; i++) {
		printf_win(win, "%s => %i\n", &DNS_table[i].hostname, DNS_table[i].ipv4);
	}
}

int DNS_get_entry(char *hostname) {
	for (int i=0; i<nb_DNS_entries; i++) {
		if (!strcmp(hostname, DNS_table[i].hostname)) return i;
	}

	return -1;
}

void DNS_add_entry(uint8 *buffer, char *hostname, uint ipv4) {
	int idx = 0;

	strcpy(DNS_table[nb_DNS_entries].hostname, hostname+1);

	uint8 c = *hostname++;

	while (c) {
		if (c == 0xC0) {
			hostname = buffer + *hostname;
			c = *hostname++;
		}

		strncpy(DNS_table[nb_DNS_entries].hostname + idx, hostname, c);
		idx += c;
		DNS_table[nb_DNS_entries].hostname[idx++] = '.';

		hostname += c;
		c = *hostname++;
	}

	DNS_table[nb_DNS_entries].hostname[idx - 1] = 0;
	DNS_table[nb_DNS_entries].ipv4 = ipv4;

//	printf("DNS: %s -> %i\n", DNS_table[nb_DNS_entries].hostname, DNS_table[nb_DNS_entries].ipv4);
	nb_DNS_entries++;
}

void DNS_send_packet(char *hostname) {
//	printf_win(win, "MAC address: %X:%X:%X:%X:%X:%X\n", E1000_adapter.MAC[0], E1000_adapter.MAC[1], E1000_adapter.MAC[2], E1000_adapter.MAC[3], E1000_adapter.MAC[4], E1000_adapter.MAC[5]);
	uint16 offset;
	uint len = strlen(hostname);

	// We want to send an UDP packet to IP address 255.255.255.255 (broadcast)
	// source port=68, dest port=67 whose message is 300 bytes long (excluding headers)
	Network *network = network_get_info();

	uint8* buffer = UDP_create_packet(network_get_DNS(), 53, UDP_PORT_DNS, 18 + len, &offset);

	// Append the HDCP header
	DNSHeader *header = (DNSHeader*)(buffer + offset);
	header->txn_id = 0x0000;
	header->flags = DNS_FLAG_QUERY;
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

	// Check that this is a response
	if (!(header->flags & DNS_FLAG_RESPONSE)) return;

	uint16 questions = switch_endian16(header->questions), answers = switch_endian16(header->answer_rr);
	DNSAlias aliases[10];
	int nb_aliases = 0;
	char c;
	uint idx = sizeof(DNSHeader);
	uint *ipv4;
	uint8 *hostname;

//	printf("%d questions, %d answers\n", questions, answers);

	// Goes through all the questions
	for (int i=0; i<questions; i++) {
		c = buffer[idx];
		while (c) {
			idx += c + 1;
			c = buffer[idx];
		}

		idx += 5;
	}

	// Goes through all the responses
	DNSAnswer *answer;
	for (int i=0; i<answers; i++) {
		answer = (DNSAnswer*)(buffer +idx);

		// Domain name alias => Store in the alias table
		if (answer->type == DNS_TYPE_ALIAS) {
			aliases[nb_aliases].hostname = buffer + (answer->name >> 8);
			if (buffer[idx + sizeof(DNSAnswer)] == 0xC0)
				aliases[nb_aliases].alias = buffer + buffer[idx + sizeof(DNSAnswer) + 1];
			else
				aliases[nb_aliases].alias = buffer + idx + sizeof(DNSAnswer);

			for (int j=0; j<nb_aliases; j++) {
				if (aliases[j].alias == aliases[nb_aliases].hostname)
					aliases[j].alias = aliases[nb_aliases].alias;
			}

			nb_aliases++;
		}
		// Domain name with an address
		else if (answer->type == DNS_TYPE_HOST_ADDRESS) {
			hostname = buffer + ((answer->name & 0xFF00) >> 8);
			ipv4 = (uint*)(buffer + idx + sizeof(DNSAnswer));
//			printf("Adding %s => %i\n", hostname, *ipv4);
			DNS_add_entry(buffer, hostname, *ipv4);

			for (int j=0; j<nb_aliases; j++) {
				if (hostname == aliases[j].alias) {
//					printf("Adding %s => %i\n", aliases[j].hostname, *ipv4);
					DNS_add_entry(buffer, aliases[j].hostname, *ipv4);
				}
			}
		}

		idx += sizeof(DNSAnswer) + switch_endian16(answer->data_length);
	}
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
