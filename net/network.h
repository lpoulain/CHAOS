#ifndef __NETWORK_H
#define __NETWORK_H

#include "libc.h"
#include "network.h"

typedef struct {
	unsigned char MAC[6];
	unsigned char router_MAC[6];
	uint IPv4;
	uint router_IPv4;
	uint subnet_mask;
	uint dns;
	uint8 status;
} Network;

void init_network();
uint8 *network_get_MAC();
uint8 *network_get_router_MAC();
uint network_get_IPv4();
uint network_get_dns();
void network_set_IPv4(uint IP);
Network *network_get_info();

#endif
