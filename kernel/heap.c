// We are using a very primitive heap right now
// All we do is to allocate memory, but never deallocate it
// There are two kernel allocations available: kmalloc and kmalloc_a
// The latter performs an allocation at the beginning of a page

#include "libc.h"
#include "heap.h"

// end is defined in the linker script.
extern uint end;

uint next_memory_block = (uint)&end;

// This doesn't do anything right now, but it may
// in the future
void init_heap() {
}

// Kernel memory allocation
uint kmalloc(uint size, uint *phys_addr) {
    uint tmp = next_memory_block;
    next_memory_block += size;

    if (phys_addr) *phys_addr = tmp;

    return tmp;
}

// Kernel memory allocation, page-aligned
uint kmalloc_a(uint size, uint *phys_addr) {
    if (next_memory_block & 0xFFF) {
        next_memory_block &= 0xFFFFF000;
        next_memory_block += 0x1000;
    }
    uint allocated = next_memory_block;
    next_memory_block += size;

    if (phys_addr) *phys_addr = allocated;
    return allocated;
}
