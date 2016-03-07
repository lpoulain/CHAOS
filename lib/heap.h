#ifndef KHEAP_H
#define KHEAP_H

#include "libc.h"
#include "display.h"

typedef struct {
	uint start;
	uint ptr;
	uint end;
	uint page_index_start;
	uint page_index_end;
	uint page_start;
	uint page_end;
	uint nb_pages;
} Heap;

void init_heap(Heap *, uint, uint, uint);
void *heap_alloc(uint, Heap *);
void *heap_alloc_pages(uint, const char *, Heap *);
void heap_free(void *, Heap *);
void heap_print(Window *, Heap *);
void *malloc(uint);
void free(void*);

void heap_check_for_corruption(Heap *h);

#endif // KHEAP_H
