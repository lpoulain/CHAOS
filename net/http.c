#include "libc.h"
#include "tcp.h"
#include "dns.h"
#include "display.h"
#include "kheap.h"

void HTTP_get(Window *win, char *hostname) {
	uint ip = DNS_query(hostname);

	if (ip == 0) {
		printf_win(win, "Cannot find the IP address for %s\n", hostname);
		return;
	}

	printf_win(win, "Establishing HTTP connection to %s (%i)...\n", hostname, ip);

	char payload[7] = "GET /\n";

	TCPConnection *connection = TCP_start_connection(ip, TCP_PORT_HTTP, (uint8*)&payload, strlen(payload));

	for (int i=0; i<2000000000; i++) {
		if (connection->status == TCP_STATUS_FIN) {
			if (connection->data) {
				uint16 idx = 0, idx_start = 0;

				while (idx < connection->data_size) {
					idx_start = idx;
					while (connection->data[idx] != 0x0A && idx < connection->data_size) idx++;

					connection->data[idx] = 0;
					printf_win(win, "%s\n", connection->data + idx_start);
					idx++;
				}

				kfree(connection->data);
			}
			else
				printf_win(win, "No data");
			return;
		}
	}	
}
