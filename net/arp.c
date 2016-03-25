#include "libc.h"
#include "network.h"
#include "ethernet.h"

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

uint8 *ARP_get_MAC(uint ipv4) {
	for (int i=0; i<nb_ARP_entries; i++) {
		if (ARP_table[i].ipv4 == ipv4) return (uint8*)&ARP_table[i].MAC;
	}
	printf("No MAC for %x\n", ipv4);
	return 0;
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

	uint8 *ip = (uint8*)&ipv4;
/*	printf("%d,%d,%d,%d => %X:%X:%X:%X:%X:%X\n",
		   ip[0], ip[1], ip[2], ip[3],
		   ARP_table[i].MAC[0], ARP_table[i].MAC[1], ARP_table[i].MAC[2], ARP_table[i].MAC[3], ARP_table[i].MAC[4], ARP_table[i].MAC[5]);*/
}

uint8 *ARP_send_packet(uint ipv4) {
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

void ARP_receive_packet(uint8 *buffer) {
	ARPPacket *header = (ARPPacket*)buffer;
	if (header->opcode == ARP_REPLY) {
		ARP_add_MAC(header->sender_ip, &header->sender_MAC);
	}
}

void init_ARP() {
	memset(&ARP_table, 0, sizeof(ARP_table));
	ARP_table[0].ipv4 = 0xFFFFFFFF;
	for (int i=0; i<6; i++)
		ARP_table[0].MAC[i] = 0xFF;
	nb_ARP_entries = 1;
}
