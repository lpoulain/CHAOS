#include "libc.h"
#include "display.h"
#include "kheap.h"
#include "isr.h"
#include "pci.h"

uint16 checksum(uint8 *addr, uint count)
{
  register uint sum = 0;

  // Main summing loop
  while(count > 1)
  {
    sum = sum + (*((uint16 *) addr))++;
    count = count - 2;
  }

  // Add left-over byte, if any
  if (count > 0)
    sum = sum + *((uint8 *) addr);

  // Fold 32-bit sum to 16 bits
  while (sum>>16)
    sum = (sum & 0xFFFF) + (sum >> 16);

  return(~sum);
}

typedef unsigned long long uint64;

typedef struct __attribute__ ((__packed__))
{
  uint8  dhost[6];  /* destination eth addr */
  uint8  shost[6];  /* source ether addr    */
  uint16 type;             /* packet type ID field */
} EthernetPacket;

#define NUM_RX_DESC 32
#define NUM_TX_DESC 32

#define REG_CTRL        0x0000
#define REG_STATUS      0x0008
#define REG_EEPROM      0x0014
#define REG_CTRL_EXT    0x0018
#define REG_IMASK       0x00D0
#define REG_RCTRL       0x0100
#define REG_RXDESCLO    0x2800
#define REG_RXDESCHI    0x2804
#define REG_RXDESCLEN   0x2808
#define REG_RXDESCHEAD  0x2810
#define REG_RXDESCTAIL  0x2818

#define REG_TCTRL       0x0400
#define REG_TXDESCLO    0x3800
#define REG_TXDESCHI    0x3804
#define REG_TXDESCLEN   0x3808
#define REG_TXDESCHEAD  0x3810
#define REG_TXDESCTAIL  0x3818

#define RCTRL_EN 	0x00000002
#define RCTRL_SBP 	0x00000004
#define RCTRL_UPE 	0x00000008
#define RCTRL_MPE 	0x00000010
#define RCTRL_8192 	0x00030000

#define REG_TIPG         0x0410      // Transmit Inter Packet Gap
#define ECTRL_SLU        0x40        //set link up uint64;

#define RCTL_EN                         (1 << 1)    // Receiver Enable
#define RCTL_SBP                        (1 << 2)    // Store Bad Packets
#define RCTL_UPE                        (1 << 3)    // Unicast Promiscuous Enabled
#define RCTL_MPE                        (1 << 4)    // Multicast Promiscuous Enabled
#define RCTL_LPE                        (1 << 5)    // Long Packet Reception Enable
#define RCTL_LBM_NONE                   (0 << 6)    // No Loopback
#define RCTL_LBM_PHY                    (3 << 6)    // PHY or external SerDesc loopback
#define RTCL_RDMTS_HALF                 (0 << 8)    // Free Buffer Threshold is 1/2 of RDLEN
#define RTCL_RDMTS_QUARTER              (1 << 8)    // Free Buffer Threshold is 1/4 of RDLEN
#define RTCL_RDMTS_EIGHTH               (2 << 8)    // Free Buffer Threshold is 1/8 of RDLEN
#define RCTL_MO_36                      (0 << 12)   // Multicast Offset - bits 47:36
#define RCTL_MO_35                      (1 << 12)   // Multicast Offset - bits 46:35
#define RCTL_MO_34                      (2 << 12)   // Multicast Offset - bits 45:34
#define RCTL_MO_32                      (3 << 12)   // Multicast Offset - bits 43:32
#define RCTL_BAM                        (1 << 15)   // Broadcast Accept Mode
#define RCTL_VFE                        (1 << 18)   // VLAN Filter Enable
#define RCTL_CFIEN                      (1 << 19)   // Canonical Form Indicator Enable
#define RCTL_CFI                        (1 << 20)   // Canonical Form Indicator Bit Value
#define RCTL_DPF                        (1 << 22)   // Discard Pause Frames
#define RCTL_PMCF                       (1 << 23)   // Pass MAC Control Frames
#define RCTL_SECRC                      (1 << 26)   // Strip Ethernet CRC

#define RCTL_BSIZE_256                  (3 << 16)
#define RCTL_BSIZE_512                  (2 << 16)
#define RCTL_BSIZE_1024                 (1 << 16)
#define RCTL_BSIZE_2048                 (0 << 16)
#define RCTL_BSIZE_4096                 ((3 << 16) | (1 << 25))
#define RCTL_BSIZE_8192                 ((2 << 16) | (1 << 25))
#define RCTL_BSIZE_16384                ((1 << 16) | (1 << 25))

#define CMD_EOP                         (1 << 0)    // End of Packet
#define CMD_IFCS                        (1 << 1)    // Insert FCS
#define CMD_IC                          (1 << 2)    // Insert Checksum
#define CMD_RS                          (1 << 3)    // Report Status
#define CMD_RPS                         (1 << 4)    // Report Packet Sent
#define CMD_VLE                         (1 << 6)    // VLAN Packet Enable
#define CMD_IDE                         (1 << 7)    // Interrupt Delay Enable

#define TCTL_EN                         (1 << 1)    // Transmit Enable
#define TCTL_PSP                        (1 << 3)    // Pad Short Packets
#define TCTL_CT_SHIFT                   4           // Collision Threshold
#define TCTL_COLD_SHIFT                 12          // Collision Distance
#define TCTL_SWXOFF                     (1 << 22)   // Software XOFF Transmission
#define TCTL_RTLC                       (1 << 24)   // Re-transmit on Late Collision
 
#define TSTA_DD                         (1 << 0)    // Descriptor Done
#define TSTA_EC                         (1 << 1)    // Excess Collisions
#define TSTA_LC                         (1 << 2)    // Late Collision
#define LSTA_TU                         (1 << 3)    // Transmit Underrun


typedef struct __attribute__((packed)) {
	uint64 addr;
	uint16 length;
	uint16 checksum;
	uint8 status;
	uint8 errors;
	uint16 special;
} E1000RxDesc;

typedef struct __attribute__((packed)) {
	uint64 addr;
	uint16 length;
	uint8 cso;
	uint8 cmd;
	uint8 status;
	uint8 css;
	uint16 special;
}  E1000TxDesc;

typedef struct {
	unsigned char MAC[6];
	unsigned char router_MAC[6];
	unsigned char *pci_bar_mem;
	E1000TxDesc *tx_descs;
	E1000RxDesc *rx_descs;
	uint rx_cur;
	uint tx_cur;
} EthernetAdapter;

EthernetAdapter E1000_adapter;

void E1000_write_command(uint16 address, uint value) {
    uint *addr = (uint*)(E1000_adapter.pci_bar_mem + address);
    *addr = value;
}

uint E1000_read_command(uint16 address) {
    uint *addr = (uint*)(E1000_adapter.pci_bar_mem + address);
    return *addr;
}


void E1000_enable_interrupt()
{
    E1000_write_command(REG_IMASK ,0x1F6DC);
    E1000_write_command(REG_IMASK ,0xff & ~4);
    E1000_read_command(0xc0);
 
}

uint flag = 0;
uint flag_FF = 0, flag_me = 0, flag_router = 0;

void E1000_handle_receive(registers_t regs) {
	uint status = E1000_read_command(0xc0);

    uint16 old_cur;
    uint8 got_packet = 0;

    while((E1000_adapter.rx_descs[E1000_adapter.rx_cur].status & 0x1))
    {
            got_packet = 1;
            uint8 *buf = (uint8 *)(E1000_adapter.rx_descs[E1000_adapter.rx_cur].addr);
            uint16 len = E1000_adapter.rx_descs[E1000_adapter.rx_cur].length;
 
            // Here you should inject the received packet into your network stack
//		    if (!flag) dump_mem(buf, 14, 280);
//	            if (flag < 10) printf("Received %x\n", buf);*/

            if (buf[0] == E1000_adapter.MAC[0] &&
            	buf[1] == E1000_adapter.MAC[1] &&
            	buf[2] == E1000_adapter.MAC[2] &&
            	buf[3] == E1000_adapter.MAC[3] &&
            	buf[4] == E1000_adapter.MAC[4] &&
            	buf[5] == E1000_adapter.MAC[5]) flag_me++;

            if (buf[0] == 0xFF &&
            	buf[1] == 0xFF &&
            	buf[2] == 0xFF &&
            	buf[3] == 0xFF &&
            	buf[4] == 0xFF &&
            	buf[5] == 0xFF) flag_FF++;

            if ((buf[0] == E1000_adapter.MAC[0] &&
            	buf[1] == E1000_adapter.MAC[1] &&
            	buf[2] == E1000_adapter.MAC[2] &&
            	buf[3] == E1000_adapter.MAC[3] &&
            	buf[4] == E1000_adapter.MAC[4] &&
            	buf[5] == E1000_adapter.MAC[5]) ||
            	(buf[0] == 0xFA &&
            	buf[1] == 0xFF &&
            	buf[2] == 0xFF &&
            	buf[3] == 0xFF &&
            	buf[4] == 0xFF &&
            	buf[5] == 0xFF)
               ) {
	            printf("%X:%X:%X:%X:%X:%X => %x\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf);
//	        	dump_mem(buf, 320, 14);
	        	ethernet_receive_packet(buf);
	    	}

/*	    	draw_ptr(flag_me, 0, 0);
	    	draw_ptr(flag_FF, 400, 0);
	    	draw_hex2(buf[0], 30*8, 0);
	    	draw_hex2(buf[1], 33*8, 0);
	    	draw_hex2(buf[2], 36*8, 0);
	    	draw_hex2(buf[3], 39*8, 0);
	    	draw_hex2(buf[4], 42*8, 0);
	    	draw_hex2(buf[5], 45*8, 0);
*/
       	    E1000_adapter.rx_descs[E1000_adapter.rx_cur].status = 0;
			old_cur = E1000_adapter.rx_cur;
			E1000_adapter.rx_cur = (E1000_adapter.rx_cur + 1) % NUM_RX_DESC;
			E1000_write_command(REG_RXDESCTAIL, old_cur);
    }    	
}

void E1000_txinit()
{
	uint *ptr;
	
//	ptr = (uint *)(kmalloc(sizeof(E1000TxDesc)*NUM_TX_DESC + 16));
//	e->tx_free = (uint8_t *)ptr;
//	if (ptr % 16 != 0)
//		ptr = (ptr + 16) - (ptr % 16);
	E1000_adapter.tx_descs = (E1000TxDesc *)kmalloc_pages(1, "Ethernet Transmission Packets");

	for(int i = 0; i < NUM_TX_DESC; i++)
	{
//		e->tx_descs[i] = (struct E1000_tx_desc *)((uintptr_t)descs + i*16);
		E1000_adapter.tx_descs[i].addr = 0;
		E1000_adapter.tx_descs[i].cmd = 0;
		E1000_adapter.tx_descs[i].status = TSTA_DD;
	}

	//give the card the pointer to the descriptors
	E1000_write_command(REG_TXDESCLO, (uint)E1000_adapter.tx_descs);
	E1000_write_command(REG_TXDESCHI, 0);

	//now setup total length of descriptors
	E1000_write_command(REG_TXDESCLEN, NUM_TX_DESC * 16);

	//setup numbers
	E1000_write_command(REG_TXDESCHEAD, 0);
	E1000_write_command(REG_TXDESCTAIL, 0);
//	e->tx_cur = 0;
	E1000_adapter.tx_cur = 0;
	/*
	E1000_write_command(REG_TCTRL, TCTL_EN
        | TCTL_PSP
        | (15 << TCTL_CT_SHIFT)
        | (64 << TCTL_COLD_SHIFT)
        | TCTL_RTLC);
*/
	E1000_write_command(REG_TCTRL,  0b0110000000000111111000011111010);
    E1000_write_command(REG_TIPG,  0x0060200A);	
}

void E1000_rxinit()
{
    uint8 * ptr;
 
    // Allocate buffer for receive descriptors. For simplicity, in my case khmalloc returns a virtual address that is identical to it physical mapped address.
    // In your case you should handle virtual and physical addresses as the addresses passed to the NIC should be physical ones
 
    ptr = (uint8 *)kmalloc_pages(1, "Ethernet receive bufferS");
 
    E1000_adapter.rx_descs = (E1000RxDesc *)ptr;
    for(int i = 0; i < NUM_RX_DESC; i++)
    {
//        rx_descs[i] = (E1000RxDesc *)((uint8 *)rx_descs + i*16);
        E1000_adapter.rx_descs[i].addr = (uint64)(uint8 *)kmalloc_pages(2, "Eth receive buffer");
        E1000_adapter.rx_descs[i].status = 0;
    }
 
    E1000_write_command(REG_TXDESCLO, (uint)((uint64)ptr >> 32) );
    E1000_write_command(REG_TXDESCHI, (uint)((uint64)ptr & 0xFFFFFFFF));
 
    E1000_write_command(REG_RXDESCLO, (uint64)ptr);
    E1000_write_command(REG_RXDESCHI, 0);
 
    E1000_write_command(REG_RXDESCLEN, NUM_RX_DESC * 16);
 
    E1000_write_command(REG_RXDESCHEAD, 0);
    E1000_write_command(REG_RXDESCTAIL, NUM_RX_DESC-1);
    E1000_adapter.rx_cur = 0;
    E1000_write_command(REG_RCTRL, RCTL_EN| RCTL_SBP| RCTL_UPE | RCTL_MPE | RCTL_LBM_NONE | RTCL_RDMTS_HALF | RCTL_BAM | RCTL_SECRC  | RCTL_BSIZE_2048);
 
}

void E1000_send_packet(uint8 *buffer, uint16 length) {
	uint packet_addr32 = (uint)buffer;
	uint64 packet_addr64 = packet_addr32;

	E1000_adapter.tx_descs[E1000_adapter.tx_cur].addr = packet_addr64;
	E1000_adapter.tx_descs[E1000_adapter.tx_cur].length = length;
	E1000_adapter.tx_descs[E1000_adapter.tx_cur].cmd = CMD_EOP | CMD_IFCS | CMD_RS | CMD_RPS;
	E1000_adapter.tx_descs[E1000_adapter.tx_cur].status = 0;

	uint8 old_cur = E1000_adapter.tx_cur;
	E1000_adapter.tx_cur = (E1000_adapter.tx_cur + 1) % NUM_TX_DESC;
	E1000_write_command(REG_TXDESCTAIL, E1000_adapter.tx_cur);
	while(!(E1000_adapter.tx_descs[old_cur].status & 0xff));
}

uint8 *E1000_get_MAC() {
	return (uint8*)&E1000_adapter.MAC;
}

void init_E1000() {
    uint8 bus, slot;

    PCIDevice *device = PCI_search_device(0x8086, 0x100E);

    if (!device) {
        printf("No networking device found\n");
        return;
    }

    E1000_adapter.pci_bar_mem = device->BAR0;
//    uint header = (uint)PCI_config_read_long(bus, slot, 0x0C, 0x0C);
//    uint test = (uint)PCI_config_read_long(bus, slot, 0x0, 0x3C);
//    printf("\nNetworking device: bus %d, slot %d, BAR0 %x\n", bus, slot, pci_bar_mem);
//    printf("[%x][%x]\n", header, test);
//    printf("[%x]\n", E1000_adapter.pci_bar_mem);
    for (uint i=0; i<6; i++) E1000_adapter.MAC[i] = E1000_adapter.pci_bar_mem[0x5400 + i];
//    printf("MAC address: %X:%X:%X:%X:%X:%X\n", MAC[0], MAC[1], MAC[2], MAC[3], MAC[4], MAC[5]);

	register_interrupt_handler(IRQ0 + device->IRQ, &E1000_handle_receive);

	// Start the network
	uint val = E1000_read_command(REG_CTRL);
	E1000_write_command(REG_CTRL, val | ECTRL_SLU);

	// Clear out the multicast filter
	for(int i = 0; i < 0x80; i++)
        E1000_write_command(0x5200 + i*4, 0);

	// Register the interrupt handler (for receiving packets)
	E1000_enable_interrupt();

	// Initialize receive transmission
    E1000_rxinit();
    // Initialize send transmission
    E1000_txinit();
}
