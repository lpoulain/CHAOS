#include "libc.h"
#include "udp.h"
#include "network.h"
#include "arp.h"

#define DHCP_REQUEST	0x01
#define DHCP_REPLY		0x02
#define DHCP_HEADER_SIZE	240
#define DHCP_OPTION_SUBNET_MASK		0x01
#define DHCP_OPTION_ROUTER			0x03
#define DHCP_OPTION_DNS				0x06
#define DHCP_OPTION_LEASE_TIME		0x51
#define DHCP_OPTION_MSG_TYPE		0x53
#define DHCP_OPTION_SERVER			0x54
#define DHCP_TYPE_DISCOVER			0x01
#define DHCP_TYPE_OFFER				0x02
#define DHCP_TYPE_REQUEST			0x03
#define DHCP_TYPE_ACK				0x05

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
		0x35, 0x01, DHCP_TYPE_DISCOVER,
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
	
	memcpy(buffer + offset + DHCP_HEADER_SIZE, (unsigned char *)&dhcp_options, 60);

//	uint16 checksum_UDP = checksum(&dhcp_packet + 42, 300);
//	printf_win(win, "%x\n", checksum_UDP);

	UDP_send_packet(buffer, 300 + offset, offset);
}

void DHCP_send_request(uint ip, uint router_ip) {
//	printf_win(win, "MAC address: %X:%X:%X:%X:%X:%X\n", E1000_adapter.MAC[0], E1000_adapter.MAC[1], E1000_adapter.MAC[2], E1000_adapter.MAC[3], E1000_adapter.MAC[4], E1000_adapter.MAC[5]);
	uint16 offset;
	uint8 *ipv4 = (uint8*)&ip, *router_ipv4 = (uint8*)&router_ip;

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
		// DHCP message type (Request)
		0x35, 0x01, DHCP_TYPE_REQUEST,
		// DHCP Server Identifier
		0x36, 0x04, router_ipv4[0], router_ipv4[1], router_ipv4[2], router_ipv4[3],
		// Requested IP address
		0x32, 0x04, ipv4[0], ipv4[1], ipv4[2], ipv4[3],
		// Hostname
		0x0C, 0x05, 0x43, 0x48, 0x41, 0x4F, 0x53,
		// Parameter request list
		0x37, 0x0D, 0x01, 0x1C, 0x02, 0x03, 0x0F, 0x06, 0x77, 0x0C, 0x2C, 0x2F, 0x1A, 0x79, 0x2A,
		// end
		0xFF,
		// padding
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};
	
	memcpy(buffer + offset + DHCP_HEADER_SIZE, (unsigned char *)&dhcp_options, 60);

//	uint16 checksum_UDP = checksum(&dhcp_packet + 42, 300);
//	printf_win(win, "%x\n", checksum_UDP);

	UDP_send_packet(buffer, 300 + offset, offset);
}

void DHCP_receive_packet(uint8* buffer, uint16 size) {
	uint8 message_type = *(buffer + sizeof(DHCPHeader) + 2);

	DHCPHeader *header = (DHCPHeader*)buffer;
	

	uint8 *options = buffer + DHCP_HEADER_SIZE;
	uint8 option_len, flag_over = 0;
	uint router_IPv4, subnet_mask, ipv4 = header->ip_you, dns;

	while (!flag_over && options - buffer < size) {
		option_len = *(options + 1);

		switch(*options) {
			case DHCP_OPTION_LEASE_TIME:
				break;
			case DHCP_OPTION_SERVER:
				break;
			case DHCP_OPTION_ROUTER:
				router_IPv4 = *((uint*)(options + 2));
				break;
			case DHCP_OPTION_SUBNET_MASK:
				subnet_mask = *((uint*)(options + 2));
				break;
			case DHCP_OPTION_DNS:
				dns = *((uint*)(options + 2));
				break;
			case 255:
				flag_over = 1;
				break;
			default:
				break;
		}

		options += option_len + 2;
	}

	// Hard coding the DNS from Level 3

	if (message_type == DHCP_TYPE_OFFER) {
		DHCP_send_request(ipv4, router_IPv4);
		return;
	}

	if (message_type != DHCP_TYPE_ACK) {
		printf("Unknown DHCP message: %X\n", message_type);
		return;
	}

	Network *network = network_get_info();
	network->IPv4 = ipv4;
	network->router_IPv4 = router_IPv4;
	network->subnet_mask = subnet_mask;
	// Hard-coding the DNS from Level 3 (209.244.0.3)
	network->dns = 0x0300F4D1;
//	network->dns = dns;

	// We want to fill the ARP table to get
	// the MAC address of the router
	if (network->router_IPv4 != 0) {
		ARP_send_packet(network->router_IPv4);
	}
}
