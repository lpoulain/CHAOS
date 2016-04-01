#include "libc.h"
#include "e1000.h"
#include "network.h"
#include "dhcp.h"
#include "arp.h"

#define NET_UNINITIALIZED	0
#define NET_MAC_ADDRESS		1
#define NET_IPV4_ADDRESS	2

Network network;

void init_network() {
	memset(&network, 0, sizeof(Network));
	init_E1000();
	init_ARP();

	uint8 *MAC = E1000_get_MAC();
	for (int i=0; i<6; i++) network.MAC[i] = MAC[i];

	DHCP_send_packet();
}

void network_set_IPv4(uint IP) {
	network.IPv4 = IP;
	network.status = NET_IPV4_ADDRESS;
}

uint network_get_IPv4() {
	return network.IPv4;
}

uint network_get_DNS() {
	return network.dns;
}

uint8 *network_get_MAC() {
	return (uint8*)&network.MAC;
}

uint8 *network_get_router_MAC() {
	return (uint8*)&network.router_MAC;
}

Network *network_get_info() {
	return &network;
}
