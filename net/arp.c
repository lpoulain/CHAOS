#include "libc.h"
#include "network.h"
#include "ethernet.h"
#include "display.h"
#include "debug.h"

#define ARP_REQUEST		0x0100
#define ARP_REPLY		0x0200
#define ARP_HEADER_SIZE		28

typedef struct __attribute__((packed)) {
	uint16 hw_type;
	uint16 protocol;
	uint8 hw_size;
	uint8 protocol_size;
	uint16 opcode;
	uint8 sender_MAC[6];
	uint sender_ip;
	uint8 target_MAC[6];
	uint target_ip;
} ARPPacket;

typedef struct {
	uint ipv4;
	uint8 MAC[6];
} ARPEntry;

ARPEntry ARP_table[100];
uint16 nb_ARP_entries = 0;

void ARP_print_table(Window *win) {
	uint8 *ip;
	for (int i=0; i<nb_ARP_entries; i++) {
		ip = (uint8*)&(ARP_table[i].ipv4);
		printf_win(win, "%d.%d.%d.%d => %X:%X:%X:%X:%X:%X\n",
				   ip[0], ip[1], ip[2], ip[3],
				   ARP_table[i].MAC[0], ARP_table[i].MAC[1], ARP_table[i].MAC[2], ARP_table[i].MAC[3], ARP_table[i].MAC[4], ARP_table[i].MAC[5]);
	}
}

void ARP_send_request(uint ipv4) {
	// Get an Ethernet packet
	uint16 offset;
	uint8 *buffer = ethernet_create_packet(ETHERNET_ARP, 0xFFFFFFFF, ARP_HEADER_SIZE + 18, &offset);

	// Append the IPv4 header
	ARPPacket *header = (ARPPacket*)(buffer + offset);
	header->hw_type = 0x0100;
	header->protocol = 0x0008;
	header->hw_size = 6;
	header->protocol_size = 4;
	header->opcode = ARP_REQUEST;
	header->sender_ip = network_get_IPv4();
	header->target_ip = ipv4;

	uint8 *MAC = network_get_MAC();
	for (int i=0; i<6; i++)
		header->sender_MAC[i] = MAC[i];

	ethernet_send_packet(buffer, offset + 18 + ARP_HEADER_SIZE);
}

uint8 *ARP_send_reply(uint ipv4, uint8 *MAC_dest) {
	// Get an Ethernet packet
	uint16 offset;
	uint8 *buffer = ethernet_create_packet(ETHERNET_ARP, ipv4, ARP_HEADER_SIZE + 18, &offset);

	// Append the IPv4 header
	ARPPacket *header = (ARPPacket*)(buffer + offset);
	header->hw_type = 0x0100;
	header->protocol = 0x0008;
	header->hw_size = 6;
	header->protocol_size = 4;
	header->opcode = ARP_REPLY;
	header->sender_ip = network_get_IPv4();
	header->target_ip = ipv4;

	uint8 *MAC_src = network_get_MAC();
	for (int i=0; i<6; i++) {
		header->sender_MAC[i] = MAC_src[i];
		header->target_MAC[i] = MAC_dest[i];
	}

	ethernet_send_packet(buffer, offset + 18 + ARP_HEADER_SIZE);
}

uint8 *ARP_get_MAC(uint ipv4) {
	for (int i=0; i<nb_ARP_entries; i++) {
		if (ARP_table[i].ipv4 == ipv4) return (uint8*)&ARP_table[i].MAC;
	}

	if (nb_ARP_entries == 1) printf("No MAC for %x\n", ipv4);

	// If the IP address is from the local network, then perform an ARP request to get its MAC address
	if ((ipv4 & 0x00FFFFFF) == (network_get_IPv4() & 0x00FFFFFF)) {
		ARP_send_request(ipv4);

		for (int j=0; j<2000000000; j++) {
			for (int i=0; i<nb_ARP_entries; i++) {
				if (ARP_table[i].ipv4 == ipv4) return (uint8*)&ARP_table[i].MAC;
			}
		}
	}

//	printf("No MAC for %x (%x != %x)\n", ipv4, ipv4 & 0x00FFFFFF, network_get_IPv4() & 0x00FFFFFF);

	// Otherwise, send the request to the router
	return (uint8*)&ARP_table[1].MAC;
}

void ARP_add_MAC(uint ipv4, uint8 *MAC) {
	int i;

	for (i=0; i<nb_ARP_entries; i++) {
		if (ARP_table[i].ipv4 == ipv4) {
			for (int j=0; j<6; j++)
				ARP_table[i].MAC[j] = MAC[j];
			return;
		}
	}

	ARP_table[nb_ARP_entries].ipv4 = ipv4;
	for (int j=0; j<6; j++)
		ARP_table[nb_ARP_entries].MAC[j] = MAC[j];

	nb_ARP_entries++;

	if (is_debug()) printf("%i => %X:%X:%X:%X:%X:%X\n",
						   ipv4,
		   				   ARP_table[i].MAC[0], ARP_table[i].MAC[1], ARP_table[i].MAC[2], ARP_table[i].MAC[3], ARP_table[i].MAC[4], ARP_table[i].MAC[5]);
}

void ARP_receive_packet(uint8 *buffer) {
	ARPPacket *header = (ARPPacket*)buffer;

	if (header->opcode == ARP_REPLY) {
		if (is_debug()) printf("=> [ARP reply %i -> %X:%X:%X:%X:%X:%X]\n", header->sender_ip, header->sender_MAC[0], header->sender_MAC[1], header->sender_MAC[2], header->sender_MAC[3], header->sender_MAC[4], header->sender_MAC[5]);
		ARP_add_MAC(header->sender_ip, (uint8*)&header->sender_MAC);
	}
	else if (header->opcode == ARP_REQUEST) {
		if (is_debug()) printf("=> [ARP request from %i]\n", header->sender_ip);
		ARP_add_MAC(header->sender_ip, (uint8*)&header->sender_MAC);
		ARP_send_reply(header->sender_ip, (uint8*)&header->sender_MAC);
	}
}

void init_ARP() {
	memset(&ARP_table, 0, sizeof(ARP_table));
	ARP_table[0].ipv4 = 0xFFFFFFFF;
	for (int i=0; i<6; i++)
		ARP_table[0].MAC[i] = 0xFF;

	nb_ARP_entries = 1;
}
