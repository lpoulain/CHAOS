#include "libc.h"
#include "kheap.h"
#include "e1000.h"
#include "network.h"
#include "ethernet.h"
#include "ipv4.h"
#include "arp.h"

#define ETHERNET_HEADER_SIZE	14

typedef struct __attribute__((packed)) {
	uint8 dest[6];
	uint8 src[6];
	uint16 type;
} EthernetHeader;

uint8 *ethernet_create_packet(uint16 protocol, uint ipv4, uint16 size, uint16 *offset) {
	size += ETHERNET_HEADER_SIZE;
	uint8 *buffer = (uint8*)kmalloc(size);
	memset(buffer, 0, size);

	uint8 *MAC = network_get_MAC();

	EthernetHeader *header = (EthernetHeader*)buffer;
	for (int i=0; i<6; i++) {
		header->dest[i] = 0xFF;
		header->src[i] = MAC[i];
	}
	header->type = protocol;

	*offset = ETHERNET_HEADER_SIZE;
	return buffer;
}

void ethernet_send_packet(uint8* buffer, uint16 size) {
	E1000_send_packet(buffer, size);
/*	if (size < 300) {
	dump_mem(buffer, 342, 14);
	for (;;);
	}*/
	kfree(buffer);
}

void ethernet_receive_packet(uint8* buffer) {
	EthernetHeader *header = (EthernetHeader*)buffer;

	switch(header->type) {
		case ETHERNET_IPV4:
			IPv4_receive_packet(buffer + ETHERNET_HEADER_SIZE);
			break;
		case ETHERNET_ARP:
			ARP_receive_packet(buffer + ETHERNET_HEADER_SIZE);
			break;
	}
}
