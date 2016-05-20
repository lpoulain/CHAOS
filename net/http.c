#include "libc.h"
#include "tcp.h"
#include "dns.h"
#include "display.h"
#include "kheap.h"

void TLS_init(Window *win, uint ip, char *hostname, uint8 payload[]);

void HTTP_TCP(Window *win, uint ip, char *hostname, uint8 payload[]) {
	TCPConnection *connection = TCP_start_connection(ip, TCP_PORT_HTTP, payload, strlen(payload));

	// It's an HTTP 1.0 request - the server sends the response and closes the TCP connection
	// We're just waiting until the connection is closed to look at the result
	for (int i=0; i<2000000000; i++) {
		if (connection->status == TCP_STATUS_FIN) {
			if (connection->data_first) {
				uint16 idx = 0, idx_start = 0;

				while (idx < connection->data_first->size) {
					idx_start = idx;
					while (connection->data_first->content[idx] != 0x0A && idx < connection->data_first->size) idx++;

					connection->data_first->content[idx] = 0;
					printf_win(win, "%s\n", connection->data_first + idx_start);
					idx++;
				}
			}
			else
				printf_win(win, "No data");
			return;
		}
	}

	TCP_cleanup_connection();
}

void _HTTP_get(Window *win, char *hostname, uint8 secure) {
	uint ip = DNS_query(hostname);

	if (ip == 0) {
		printf_win(win, "Cannot find the IP address for %s\n", hostname);
		return;
	}

	if (secure)
		printf_win(win, "Establishing HTTPS connection to %s (%i)...\n\n", hostname, ip);
	else
		printf_win(win, "Establishing HTTP connection to %s (%i)...\n\n", hostname, ip);

	char *payload = (char*)kmalloc(23 + strlen(hostname) + 1);
	strcpy(payload, "GET / HTTP/1.0\nHost: ");
	strcpy(payload+ 21, hostname);
	payload[21 + strlen(hostname)] = '\n';
	payload[22 + strlen(hostname)] = '\n';
	payload[23 + strlen(hostname)] = 0;

	if (secure) TLS_init(win, ip, hostname, payload);
	else HTTP_TCP(win, ip, hostname, payload);

	kfree(payload);
}

void HTTP_get(Window *win, char *hostname) {
	_HTTP_get(win, hostname, 0);
}

void HTTPS_get(Window *win, char *hostname) {
	_HTTP_get(win, hostname, 1);
}
