#ifndef __ARP_H
#define __ARP_H

#include "libc.h"
#include "display.h"

uint8 *ARP_get_MAC(uint ipv4);
uint8 *ARP_send_packet(uint ipv4);
void ARP_receive_packet(uint8 *buffer);
void init_ARP();
void ARP_print_table(Window *win);
void ARP_add_MAC(uint ipv4, uint8 *MAC);

#endif
