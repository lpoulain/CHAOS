#include "libc.h"
#include "udp.h"
#include "network.h"

#define DHCP_REQUEST	0x01
#define DHCP_REPLY		0x02
#define DHCP_HEADER_SIZE	240
#define DHCP_OPTION_SUBNET_MASK		0x01
#define DHCP_OPTION_ROUTER			0x03
#define DHCP_OPTION_DNS				0x06
#define DHCP_OPTION_LEASE_TIME		0x51
#define DHCP_OPTION_MSG_TYPE		0x53
#define DHCP_OPTION_SERVER			0x54


typedef struct __attribute__((packed)) {
	uint8 msg_type;
	uint8 hardware_type;
	uint8 hardware_addr_len;
	uint8 hops;
	uint txn_id;
	uint16 seconds;
	uint16 flags;
	uint ip_client;
	uint ip_you;
	uint ip_next_server;
	uint ip_relay_agent;
	uint8 MAC[6];
	uint8 padding[10];
	uint8 server_hostname[64];
	uint8 boot_filename[128];
	uint magic_cookie;
} DHCPHeader;

void DHCP_send_packet() {
//	printf_win(win, "MAC address: %X:%X:%X:%X:%X:%X\n", E1000_adapter.MAC[0], E1000_adapter.MAC[1], E1000_adapter.MAC[2], E1000_adapter.MAC[3], E1000_adapter.MAC[4], E1000_adapter.MAC[5]);
	uint16 offset;

	// We want to send an UDP packet to IP address 255.255.255.255 (broadcast)
	// source port=68, dest port=67 whose message is 300 bytes long (excluding headers)
	uint8* buffer = UDP_create_packet(0xFFFFFFFF, UDP_PORT_DHCP, 67, 300, &offset);

	// Append the HDCP header
	DHCPHeader *header = (DHCPHeader*)(buffer + offset);

	header->msg_type = DHCP_REQUEST;
	header->hardware_type = 0x01;
	header->hardware_addr_len = 0x06;
	header->txn_id = 0x36823B0C;

	uint8 *MAC = network_get_MAC();
	for (int i=0; i<6; i++) header->MAC[i] = MAC[i];
	header->magic_cookie = 0x63538263;

	unsigned char dhcp_options[60] = {
		// DHCP message type (Discover)
		0x35, 0x01, 0x01,
		// Requested IP address
//		0x32, 0x04, 0xC0, 0xA8, 0x00, 0x78,
		// Hostname
		0x0C, 0x05, 0x43, 0x48, 0x41, 0x4F, 0x53,
		// Parameter request list
		0x37, 0x0D, 0x01, 0x1C, 0x02, 0x03, 0x0F, 0x06, 0x77, 0x0C, 0x2C, 0x2F, 0x1A, 0x79, 0x2A,
		// end
		0xFF,
		// padding
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00
	};



/*	unsigned char arp_packet[60] = {
		0xAC, 0x87, 0xA3, 0x00, 0xFB, 0xBA,
		0x08, 0x00, 0x27, 0x60, 0x22, 0x02,
		0x08, 0x06,

		0x00, 0x01,
		0x08, 0x00,
		0x06, 0x04, 0x00, 0x02,
		0x08, 0x00, 0x27, 0x60, 0x22, 0x02,
		0xC0, 0xA8, 0x00, 0xFF,
		0xAC, 0x87, 0xA3, 0x00, 0xFB, 0xBA,
		0xC0, 0xA8, 0x00, 0x64,

		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00
	};*/

		unsigned char dhcp_packet[300] = {
			/*
			// Ethernet header
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0x08, 0x00, 0x27, 0x60, 0x22, 0x02,
			0x08, 0x00,
			// IPv4 header
			0x45, 0x10,
			0x01, 0x48,		// length
			0x00, 0x00, 0x00, 0x00,
			0x80,
			0x11,	// UDP
			0x39, 0x96,		// IP checksum
			0x00, 0x00, 0x00, 0x00,		// source
			0xFF, 0xFF, 0xFF, 0xFF,		// dest
			// UDP header
			0x00, 0x44, 0x00, 0x43, 0x01, 0x34,
			0x4F, 0x3B,	// UDP checksum
			*/
			// DHCP request
			0x01,
			0x01,
			0x06, 0x00,
				// txn ID
			0x0C, 0x3B, 0x82, 0x36,
				// flags
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00,
				// MAC address
			0x08, 0x00, 0x27, 0x60, 0x22, 0x02,
				// padding
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00,	// padding
				// server hostname
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				// boot filename
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				// magic cookie: DHCP
			0x63, 0x82, 0x53, 0x63,
			0x35, 0x01, 0x01,
			0x32, 0x04, 0xC0, 0xA8, 0x00, 0x78,
			0x0C, 0x04, 0x6B, 0x61, 0x6C, 0x69,
			0x37, 0x0D, 0x01, 0x1C, 0x02, 0x03, 0x0F, 0x06, 0x77, 0x0C, 0x2C, 0x2F, 0x1A, 0x79, 0x2A,
			0xFF,	// end
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00
		};

	memcpy(buffer + offset + DHCP_HEADER_SIZE, (unsigned char *)&dhcp_options, 60);

//	uint16 checksum_UDP = checksum(&dhcp_packet + 42, 300);
//	printf_win(win, "%x\n", checksum_UDP);

	UDP_send_packet(buffer, 300 + offset, offset);
}

void DHCP_receive_packet(uint8* buffer, uint16 size) {
	DHCPHeader *header = (DHCPHeader*)buffer;
	Network *network = network_get_info();
	network_set_IPv4(header->ip_you);

	uint8 *options = buffer + DHCP_HEADER_SIZE;
	uint8 option_len, flag_over = 0;

	while (!flag_over && options - buffer < size) {
		option_len = *(options + 1);

		switch(*options) {
			case DHCP_OPTION_LEASE_TIME:
				break;
			case DHCP_OPTION_SERVER:
				break;
			case DHCP_OPTION_ROUTER:
				network->router_IPv4 = *((uint*)(options + 2));
				break;
			case DHCP_OPTION_SUBNET_MASK:
				network->subnet_mask = *((uint*)(options + 2));
				break;
			case DHCP_OPTION_DNS:
				network->dns = *((uint*)(options + 2));
				break;
			case 255:
				flag_over = 1;
				break;
			default:
				break;
		}

		options += option_len + 2;
	}

	// We want to fill the ARP table to get
	// the MAC address of the router
	if (network->router_IPv4 != 0) {
		ARP_send_packet(network->router_IPv4);
	}
}