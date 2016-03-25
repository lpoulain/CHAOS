#include "libc.h"
#include "pci.h"
#include "kernel.h"
#include "display.h"
#include "kheap.h"

extern int PCI_get_device_name(uint vendor, uint device, char **vendor_name, char **device_name);

PCIDevice *PCI_chain;

uint16 PCI_config_read_word(uint8 bus, uint8 slot, uint8 func, uint8 offset)
{
    uint address;
    uint lbus  = (uint)bus;
    uint lslot = (uint)slot;
    uint lfunc = (uint)func;
    uint16 tmp = 0;
 
    /* create configuration address as per Figure 1 */
    address = (uint)((lbus << 16) | (lslot << 11) |
              (lfunc << 8) | (offset & 0xfc) | ((uint)0x80000000));
 
    /* write out the address */
    outportl(0xCF8, address);
    /* read in the data */
    /* (offset & 2) * 8) = 0 will choose the first word of the 32 bits register */
    tmp = (uint16)((inportl(0xCFC) >> ((offset & 2) * 8)) & 0xffff);
    return (tmp);
}

uint PCI_config_read_long(uint8 bus, uint8 slot, uint8 func, uint8 offset) {
    uint address;
    uint lbus  = (uint)bus;
    uint lslot = (uint)slot;
    uint lfunc = (uint)func;

    address = (uint)((lbus << 16) | (lslot << 11) |
              (lfunc << 8) | (offset & 0xfc) | ((uint)0x80000000));

    outportl(0xCF8, address);
    return inportl(0xCFC);
}

int PCI_get_vendor(uint8 bus, uint8 slot, uint *vendor, uint *device)
{
//    uint16 vendor, device;
    /* try and read the first configuration register. Since there are no */
    /* vendors that == 0xFFFF, it must be a non-existent device. */
    if ((*vendor = PCI_config_read_word(bus,slot,0,0)) != 0xFFFF) {
       *device = PCI_config_read_word(bus,slot,0,2);
       return 1;
    }

    return 0;
}

PCIDevice *PCI_search_device(uint16 vendor_id, uint16 device_id) {
    PCIDevice *device = PCI_get_devices();

    while (device) {
        if (device->vendor_id == vendor_id && device->device_id == device_id)
            return device;
        device = device->next;
    }

    return 0;
}

PCIDevice *PCI_get_devices() {
    return PCI_chain;
}

void init_PCI() {
    PCI_chain = 0;
    PCIDevice *device, *last_device;
    uint vendor_id, device_id;
    char *vendor_name, *device_name;
    int idx;

    for (uint bus=0; bus<256; bus++) {
        for (uint slot=0; slot<32; slot++) {
            if (PCI_get_vendor(bus, slot, &vendor_id, &device_id)) {

                device = (PCIDevice*)kmalloc(sizeof(PCIDevice));
                device->bus = bus;
                device->slot = slot;
                device->vendor_id = vendor_id;
                device->device_id = device_id;
                device->next = 0;

                if (PCI_chain == 0) {
                    last_device = device;
                    PCI_chain = device;
                } else {
                    last_device->next = device;
                    last_device = device;
                }

                PCI_get_device_name(vendor_id, device_id, &device->vendor_name, &device->device_name);
                device->BAR0 = (unsigned char*)PCI_config_read_long(bus, slot, 0, 0x10);
                device->IRQ = (uint8)(PCI_config_read_long(bus, slot, 0, 0x3C) & 0xFF);
            }
        }
    }

}
