#ifndef __TCP_H
#define __TCP_H

#include "libc.h"
#include "display.h"

#define TCP_PORT_HTTP					80

#define TCP_STATUS_HANDSHAKE_SYN 		1
#define TCP_STATUS_HANDSHAKE_SYN_ACK	2
#define TCP_STATUS_HANDSHAKE_ACK 		3
#define TCP_STATUS_TRANSFER_PUSH		4
#define TCP_STATUS_TRANSFER_ACK			5
#define TCP_STATUS_FIN					6

typedef struct {
	uint16 sport;
	uint16 dport;
	int status;
	uint8 *payload;
	uint16 payload_size;
	uint8 *data;
	uint16 data_size;
} TCPConnection;

TCPConnection *TCP_start_connection(uint ipv4, uint16 dport, uint8 *payload, uint16 payload_size);
void TCP_receive_packet(uint ipv4_from, uint8 *buffer, uint16 size);

#endif
