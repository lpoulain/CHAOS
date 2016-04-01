#ifndef __ICMP_H
#define __ICMP_H

#define ICMP_TYPE_ECHO_REQUEST	0x08
#define ICMP_TYPE_ECHO_REPLY	0x00
#define ICMP_TYPE_ECHO_UNREACHABLE	0x03

void ICMP_send_packet(uint ipv4, uint ps_id);
void ICMP_receive_packet(uint ipv4, uint8* buffer_ping, uint16 size);
void ICMP_register_reply(uint ps_id);
void ICMP_unregister_reply(uint ps_id);
uint8 ICMP_check_response(uint ps_id);

#endif
