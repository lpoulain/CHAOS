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

	uint8 *MAC_src = network_get_MAC();
	uint8 *MAC_dst = ARP_get_MAC(ipv4);

	EthernetHeader *header = (EthernetHeader*)buffer;
	for (int i=0; i<6; i++) {
		header->dest[i] = MAC_dst[i];
		header->src[i] = MAC_src[i];
	}
	header->type = protocol;

	*offset = ETHERNET_HEADER_SIZE;
	return buffer;
}

void ethernet_send_packet(uint8* buffer, uint16 size) {
	EthernetHeader *header = (EthernetHeader*)buffer;
//	printf("%X:%X:%X:%X:%X:%X <=\n", header->dest[0], header->dest[1], header->dest[2], header->dest[3], header->dest[4], header->dest[5]);
	E1000_send_packet(buffer, size);

	kfree(buffer);
}

void ethernet_receive_packet(uint8* buffer, uint16 size) {
	EthernetHeader *header = (EthernetHeader*)buffer;
	uint8 *MAC_router;
	IPv4Header *header_ip;

	switch(header->type) {
		case ETHERNET_IPV4:
			MAC_router = network_get_router_MAC();
			if (header->src[0] != MAC_router[0] ||
				header->src[1] != MAC_router[1] ||
				header->src[2] != MAC_router[2] ||
				header->src[3] != MAC_router[3] ||
				header->src[4] != MAC_router[4] ||
				header->src[5] != MAC_router[5]) {

				header_ip = (IPv4Header*)(buffer + ETHERNET_HEADER_SIZE);
				ARP_add_MAC(header_ip->ip_src, (uint8*)&header->src);
			}
			IPv4_receive_packet(buffer + ETHERNET_HEADER_SIZE, size - ETHERNET_HEADER_SIZE);
			break;
		case ETHERNET_ARP:
			ARP_receive_packet(buffer + ETHERNET_HEADER_SIZE);
			break;
	}
}
