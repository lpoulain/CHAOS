#ifndef KHEAP_H
#define KHEAP_H

void init_heap();
uint kmalloc(uint size, uint *phys_addr);
uint kmalloc_a(uint size, uint *phys_addr);

#endif // KHEAP_H
