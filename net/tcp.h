#ifndef __TCP_H
#define __TCP_H

#include "libc.h"
#include "display.h"

#define TCP_PORT_HTTP					80
#define TCP_PORT_HTTPS					443

#define TCP_STATUS_HANDSHAKE_SYN 		1
#define TCP_STATUS_HANDSHAKE_SYN_ACK	2
#define TCP_STATUS_HANDSHAKE_ACK 		3
#define TCP_STATUS_TRANSFER_PUSH		4
#define TCP_STATUS_TRANSFER_ACK			5
#define TCP_STATUS_FIN					6

typedef struct tcp_data {
	uint8 *content;
	uint16 size;
	struct tcp_data *next;
} TCPData;

typedef struct {
	uint ipv4;
	uint16 sport;
	uint16 dport;
	int status;
	uint8 *payload;
	uint16 payload_size;
	TCPData *data_first;
	TCPData *data_last;
	uint sequence_nb;
	uint ack_nb;
} TCPConnection;

TCPConnection *TCP_start_connection(uint ipv4, uint16 dport, uint8 *payload, uint16 payload_size);
void TCP_receive_packet(uint ipv4_from, uint8 *buffer, uint16 size);
void TCP_send(uint8 payload[], uint size);
void TCP_cleanup_connection();

#endif
