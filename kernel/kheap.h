#include "libc.h"
#include "heap.h"
#include "display.h"

void init_kheap();
void *kmalloc_pages(uint, const char *);
void *kmalloc(uint);
void kfree(void*);
void kheap_print(Window *);

extern Heap kheap;
