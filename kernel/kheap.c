// This is the kernel heap - the heap used by the kernel to allocate
// kernel structures, as well as process heaps

#include "libc.h"
#include "heap.h"
#include "display.h"

Heap kheap;
extern uint end;

void init_kheap() {
    // - end of the used memory -> 0x800000 (8 Mb): used for small objects
    // - 0x800000 -> 0x1000000 (8 Mb to 16 Mb): used to allocate whole pages
	init_heap(&kheap, (uint)&end, 0x800000, 0x1000000);
}

void* kmalloc_pages(uint nb_pages, const char *name) {
	heap_alloc_pages(nb_pages, name, &kheap);
}

void* kmalloc(uint nb_bytes) {
	heap_alloc(nb_bytes, &kheap);
}

void kfree(void *ptr) {
	heap_free(ptr, &kheap);
}

void kheap_print(Window *win) {
	heap_print(win, &kheap);
}

void kheap_print_pages(Window *win) {
	heap_print_pages(win, &kheap);
}

uint kheap_free_space() {
	return heap_free_space(&kheap);
}

void kheap_check_for_corruption(const char *msg) {
	heap_check_for_corruption(&kheap, msg);
}
