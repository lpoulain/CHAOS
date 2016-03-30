#ifndef __ICMP_H
#define __ICMP_H

void ICMP_send_packet(uint ipv4);
void ICMP_receive_packet(uint ipv4, uint8* buffer_ping, uint16 size);

#endif
