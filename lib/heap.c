/*
The heap is managing two types of allocation requests: blocks of pages and everything smaller

THE HEAP CLASS

The Heap class is designed to be instantiated for the kernel and for each process. The kernel
can "allocate" (more like reserves) some memory inside its heap and give it to a process that will
use it as its own heap when it can allocate/free memory for its own use.

SMALL OBJECT ALLOCATIONS

The small objects heap is loosely based on http://g.oswego.edu/dl/html/malloc.html in the sense
that it's a series of blocks (occupied or empty) surrounded by a header and a footer. The objects
are however NOT stored in bins right now, so scanning for an empty block might take some time as
the heap grows.

2) Pages block allocations

The OS needs to allocates several times one or several 4K pages - process stacks, disk directory,
files loaded into memory, etc.
The algorithm from http://g.oswego.edu/dl/html/malloc.html requires some objects before and after
the allocation. Those objects would be at the end of the previous pages and the beginning of the
next page, which would leave large holes in the heap.

Page blocks have their own part of the heap (Heap.page_index_start to Heap.page_end). It starts with
a page index (composed of as many HeapPageIndex as we have pages) followed by the pages

Right now the page heap is using 8 Mb or 2048 pages. 2 of them are used for the page index and 2046 for
the pages themselves.
*/

#include "libc.h"
#include "kernel.h"
#include "heap.h"
#include "display.h"

#define HEAP_MAGIC 0x9F4372
#define NO_MORE_SPACE 0xFFFFFFFF

extern uint rodata;
extern Window gui_debug_win;

Heap *default_heap;

typedef struct {
    uint magic:24;
    uint occupied:8;
    uint size;
} HeapHeader;

typedef struct {
    uint magic;
    HeapHeader *header;
} HeapFooter;

// This is the
// For de
typedef struct __attribute__((packed)) {
    uint nb_pages:12;
    uint name:18;       // For debugging purposes, the allocated page blocks can have a name
    uint occupied:1;
    uint active:1;
} HeapPageIndex;

#include "heap.h"

//uint next_memory_block = (uint)&end;

void check_heap_entry(HeapHeader *header) {
    int OK = 1;
    
    if (header->magic != HEAP_MAGIC) {
        printf("Heap entry at %x corrupted - invalid header magic number\n", header);
        OK = 0;
    }

    HeapFooter *footer = (HeapFooter*)((uint)header + sizeof(HeapHeader) + header->size);

    if (footer->magic != HEAP_MAGIC) {
        printf("Heap entry at %x corrupted - invalid footer magic number\n", header);
        OK = 0;
    }

    if (!OK) {
        stack_dump();
        for (;;);
    }
}

void heap_check_for_corruption(Heap *h) {
    HeapHeader *header = (HeapHeader*)h->start;
    while (header >= (HeapHeader*)h->start && header < (HeapHeader*)h->end) {
        check_heap_entry(header);
        header = (HeapHeader*)((uint)header + header->size + sizeof(HeapHeader) + sizeof(HeapFooter));
    }
    if ((uint)header != h->end) {
        printf("Error: last entry corrupted\n");
        stack_dump();
        for (;;);
    }
}

void init_heap(Heap *h, uint start, uint pages, uint end) {
    // Set the default heap
    default_heap = h;

    // Initialized the Heap object
    h->start = start;
    if (h->start % 0x1000 != 0) h->start += -h->start % 0x1000 + 0x1000;
//    next_memory_block = h->start;
    h->ptr = h->start;
    h->end = pages;
    h->page_index_start = pages;
    h->page_end = end;

    // Initializes the small objects section - create one empty block
    HeapHeader *header = (HeapHeader*)h->start;
    HeapFooter *footer = (HeapFooter*)(h->end - sizeof(HeapFooter));
    header->magic = HEAP_MAGIC;
    header->occupied = 0;
    header->size = (uint)footer - (uint)header - sizeof(HeapHeader);
    footer->magic = HEAP_MAGIC;
    footer->header = header;
    check_heap_entry(header);

    // Initializes the page block section
    h->nb_pages = (h->page_end - h->page_index_start) / (4096 + 4);
    h->page_index_end = h->page_index_start + h->nb_pages * sizeof(HeapPageIndex);
    h->page_start = h->page_index_start + h->nb_pages * sizeof(HeapPageIndex);
    if (h->page_start % 0x1000 != 0) h->page_start = h->page_start -(h->page_start % 0x1000) + 0x1000;

    HeapPageIndex *idx=(HeapPageIndex*)h->page_index_start;

    for (int i=0; i<h->nb_pages; i++) {
        idx->occupied = 0;
        idx->nb_pages = 1;
        idx->active = 0;
        idx++;
    }

    idx=(HeapPageIndex*)h->page_index_start;
    idx->nb_pages = h->nb_pages;
    idx->active = 1;
}

void *heap_alloc(uint nb_bytes, Heap *h) {
    HeapHeader *header = (HeapHeader*)h->start;
    HeapFooter *footer = (HeapFooter*)(h->start + sizeof(HeapHeader) + header->size);
    HeapHeader *candidate, *next_header;
    uint candidate_size = NO_MORE_SPACE;

    while (header->magic == HEAP_MAGIC) {
        next_header = (HeapHeader*)((uint)header + header->size + sizeof(HeapHeader) + sizeof(HeapFooter));

        // Block is used - next
        if (header->occupied) {
            header = next_header;
            continue;
        }

        // Block is too small - next
        if (header->size < nb_bytes) {
            header = next_header;
            continue;
        }

        // The block is juuust right - let's take it
        if (header->size == nb_bytes) {
            header->occupied = 1;
            return (void*)((uint)header + sizeof(HeapHeader));
        }

        // The block isn't perfect but better than
        // what we had, keep it
        if (header->size < candidate_size) {
            candidate = header;
            candidate_size = header->size;
        }

        header = next_header;
    }

    // Out of memory
    if (candidate_size == NO_MORE_SPACE) {
        printf("No more space\n");
        heap_print(&gui_debug_win, h);

        dump_mem((void*)h->start, 200, 20);

        for (;;);
        return 0;
    }

    // The block is larger than what we want, but too
    // small to be broken down. Let's use it as is
    if (candidate_size - nb_bytes < sizeof(HeapHeader) + sizeof(HeapFooter) + 2) {
        candidate->occupied = 1;
        return (void*)((uint)candidate + sizeof(HeapHeader));
    }

    // We have a block, let's break it
    candidate->occupied = 1;
    candidate->size = nb_bytes;
    footer = (HeapFooter*)((uint)candidate + sizeof(HeapHeader) + nb_bytes);
    footer->magic = HEAP_MAGIC;
    footer->header = candidate;

    header = (HeapHeader*)((uint)candidate + sizeof(HeapHeader) + nb_bytes + sizeof(HeapFooter));
    header->occupied = 0;
    header->magic = HEAP_MAGIC;
    header->size = candidate_size - nb_bytes - sizeof(HeapHeader) - sizeof(HeapFooter);
    footer = (HeapFooter*)((uint)header + sizeof(HeapHeader) + header->size);
    footer->magic = HEAP_MAGIC;
    footer->header = header;

    check_heap_entry(candidate);
    check_heap_entry(header);

    return (void*)((uint)candidate + sizeof(HeapHeader));
}

void heap_free_small_object(uint ptr, Heap *p) {
    HeapHeader *header2, *header = (HeapHeader*)(ptr - 8);
    HeapFooter *footer;

    if (header->magic != HEAP_MAGIC) return;

    header->occupied = 0;

    // Look for the next block. If it is empty, merge the two
    header2 = (HeapHeader*)((uint)header + sizeof(HeapHeader) + header->size + sizeof(HeapFooter));
    if (header2->magic == HEAP_MAGIC && header2->occupied == 0) {
        header->size += header2->size + sizeof(HeapFooter) + sizeof(HeapHeader);

        footer = (HeapFooter*)((uint)header2 + sizeof(HeapHeader) + header2->size);
        footer->header = header;
    }

    // Look for the previous block. If it is empty, merge the two
    footer = (HeapFooter*)((uint)header - sizeof(HeapFooter));
    if (footer->magic == HEAP_MAGIC) {
        header2 = footer->header;
        if (header2->occupied == 0) {
            header2->size += header->size + sizeof(HeapHeader) + sizeof(HeapFooter);
            footer = (HeapFooter*)((uint)header + sizeof(HeapHeader) + header->size);
            footer->header = header2;
            header = header2;
        }
    }

    check_heap_entry(header);

    // Set the block as free
    header->occupied = 0;
}

void *heap_alloc_pages(uint nb_requested_pages, const char *name, Heap *h) {
    HeapPageIndex *candidate, *idx = (HeapPageIndex*)h->page_index_start;
    uint candidate_pages = NO_MORE_SPACE;

    while ((uint)idx < h->page_index_end) {
        // The block is occupied - next
        if (idx->occupied == 1) {
            idx += idx->nb_pages;
            continue;
        }

        // The block is too small - next
        if (idx->nb_pages < nb_requested_pages) {
            idx += idx->nb_pages;
            continue;
        }

        // The block is juuust right. Let's take it
        if (idx->nb_pages == nb_requested_pages) {
            idx->occupied = 1;
            idx->name = (uint)name - (uint)&rodata;
            uint page_idx = ((uint)candidate - h->page_index_start) / sizeof(HeapPageIndex);
            return (void*)(h->page_start + 0x1000 * page_idx);
        }

        // The block isn't perfect but better than
        // what we had, keep it
        if (idx->nb_pages < candidate_pages) {
            candidate_pages = idx->nb_pages;
            candidate = idx;
        }

        idx += idx->nb_pages;
    }

    // We couldn't find a block
    if (candidate_pages == NO_MORE_SPACE) {
        printf("Memory full - No more pages");
        return 0;
    }

    // break the block
    idx = candidate + nb_requested_pages;
    idx->nb_pages = candidate->nb_pages - nb_requested_pages;
    idx->occupied = 0;
    idx->active = 1;
    candidate->nb_pages = nb_requested_pages;
    candidate->occupied = 1;
    candidate->active = 1;
    candidate->name = (uint)name - (uint)&rodata;

    uint page_idx = ((uint)candidate - h->page_index_start) / sizeof(HeapPageIndex);
    return (void*)(h->page_start + 0x1000 * page_idx);
}

void heap_free_pages(uint ptr, Heap *h) {
    // Make sure the address belongs to a page block
    if (ptr < h->page_start || ptr >= h->page_end) return;

    HeapPageIndex *idx_next, *idx = (HeapPageIndex*)h->page_index_start + (ptr - h->page_start) / 0x1000;

    // If we are in the middle of a block, do nothing
    if (idx->active == 0) return;

    // Mark the page as free
    idx->occupied = 0;

    // Check the next page block
    // If it is also free, merge the two blocks
    idx_next = idx + idx->nb_pages;
    if ((uint)idx_next < h->page_index_end) {
        if (idx_next->occupied == 0) {
            idx->nb_pages += idx_next->nb_pages;
            idx_next->active = 0;
        }
    }

    // Check the previous page block
    // If it is also free, merge the two blocks
    idx_next = idx - 1;
    if ((uint)idx_next > h->page_index_start) {
        while ((uint)idx_next > h->page_index_start && idx_next->active == 0) idx_next--;
        if (idx_next->occupied == 0) {
            idx_next->nb_pages += idx->nb_pages;
            idx->active = 0;
            idx = idx_next;
        }
    }

    // Mark all the pages but the first one one as passive
    for (int i=1; i<idx->nb_pages; i++) {
        idx++;
        idx->active = 0;
    }
}

void heap_print(Window *win, Heap *h) {

//    printf("%x-%x / %x-%x (%d pages)\n", h->page_index_start, h->page_index_end, h->page_start, h->page_end, h->nb_pages);
    printf_win(win, "Small objects:\n");
    HeapHeader *header = (HeapHeader*)h->start;
    HeapFooter *footer = (HeapFooter*)(h->start + sizeof(HeapHeader) + header->size);
    HeapHeader *candidate, *next_header;

    while (header->magic == HEAP_MAGIC) {
        printf_win(win, "%x -> %x (%d bytes) - ", header, (uint)header + sizeof(HeapHeader) + header->size + sizeof(HeapFooter), header->size);
        if (header->occupied) printf_win(win, "USED\n"); else printf_win(win, "free\n");

        header = (HeapHeader*)((uint)header + header->size + sizeof(HeapHeader) + sizeof(HeapFooter));
    }

    printf_win(win, "Page blocks:\n");
    HeapPageIndex *idx = (HeapPageIndex*)h->page_index_start;
    uint start, end;

    while ((uint)idx < h->page_index_end) {
        if (idx->active == 1) {
            start = h->page_start + (((uint)idx - h->page_index_start) / sizeof(HeapPageIndex)) * 0x1000;
            end = start + 0x1000 * idx->nb_pages;

            printf_win(win, "[%x -> %x] (%d pages)", start, end, idx->nb_pages);
            if (idx->occupied == 1  ) printf_win(win, " %s (USED)\n", (uint)&rodata + idx->name);
            else printf_win(win, " (free)\n");
        }

        idx ++;
    }
}

// Kernel memory allocation
/*
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
*/

void heap_free(void *ptr, Heap *h) {
    uint pointer = (uint)ptr;

    // Small object deallocation
    // No such thing for now
    if (pointer >= h->start && pointer < h->end) {
        heap_free_small_object(pointer, h);
        return;
    }

    // Page deallocation
    if (pointer >= h->page_start && pointer < h->page_end) {
        heap_free_pages(pointer, h);
        return;
    }

    // Somewhere else - do nothing
    printf("Error, trying to free %x outside of the heap range\n", pointer);
}

void *malloc(uint nb_bytes) {
    heap_alloc(nb_bytes, default_heap);
}

void free(void *ptr) {
    heap_free(ptr, default_heap);
}
