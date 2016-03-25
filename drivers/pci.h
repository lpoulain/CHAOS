#include "libc.h"

typedef struct PCI_device_t {
	uint16 vendor_id;
	uint16 device_id;
	char *vendor_name;
	char *device_name;
	unsigned char *BAR0;
	struct PCI_device_t *next;
	uint8 bus;
	uint8 slot;
	uint8 IRQ;
} PCIDevice;

PCIDevice *PCI_get_devices();
PCIDevice *PCI_search_device(uint16 vendor_id, uint16 device_id);
void PCI_init();
