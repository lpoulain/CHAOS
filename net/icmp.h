#ifndef __ICMP_H
#define __ICMP_H

void ICMP_send_packet(uint ipv4);
void ICMP_receive_packet(uint ipv4, uint8* buffer_ping, uint16 size);
void ICMP_register_reply(uint ps_id, uint16 id);
void ICMP_unregister_reply(uint ps_id);
uint8 ICMP_has_response(uint ps_id);

#endif
