#include "libc.h"
#include "kheap.h"
#include "kernel.h"
#include "virtualmem.h"
#include "display.h"
#include "isr.h"

// The kernel's page directory
//PageDirectory *kernel_directory=0;

// The current page directory;
PageDirectory *current_page_directory=0;

void print_page_directory(PageDirectory *, Window *);
extern void copy_physical_page(uint, uint);
extern void stack_dump();

// The frame bitmap is an array of bits that indicates
// whether a frame is free or not
// (frame = physical memory page)
uint *frame_bitmap;
uint nb_frames;

// This page is used whenever someone tries to access a page which
// is not mapped or forbidden
void *forbidden_page;


static void page_fault(registers_t reg);


// Turns the bit from the frame bitmap
static void set_frame(uint frame_addr) {
    uint frame_base_addr = frame_addr / 0x1000;
    uint idx = frame_base_addr / 32;
    uint offset = frame_base_addr % 32;
    frame_bitmap[idx] |= (0x1 << offset);
}

static void clear_frame(uint frame_addr) {
    uint frame_base_addr = frame_addr / 0x1000;
    uint idx = frame_base_addr / 32;
    uint offset = frame_base_addr % 32;
    frame_bitmap[idx] |= ~(0x1 << offset);
}

static uint check_frame(uint frame_addr) {
    uint frame_base_addr = frame_addr / 0x1000;
    uint idx = frame_base_addr / 32;
    uint offset = frame_base_addr % 32;
    return frame_bitmap[idx] & (0x1 << offset);
}

static uint first_free_frame() {
    uint frame_block;
    // Goes through the frame array
    for (uint i=0; i<nb_frames/32; i++) {
        // If the 32-bit block isn't full
        if (frame_bitmap[i] != 0xFFFFFFFF) {
            frame_block = frame_bitmap[i];
            // We go through all the bits in the 32-bit block
            for (uint j=0; j<32; j++) {
                uint bit = 0x1 << j;
                if (!(frame_block & bit)) {
                    return i*32 + j;
                }
            }
        }
    }

    // The whole memory is full
    return -1;
}

void map_to_first_available(uint virtual_addr, int is_user, int is_writeable) {
    PageTableEntry *pte = get_PTE(virtual_addr, current_page_directory, 1);
    
    // The page already has a frame, nothing to do
    if (pte->frame != 0) return;

    // Find the first free frame available
    uint frame = first_free_frame();

    // If there isn't any, we're out of memory
    if (frame == -1) {
        printf("Memory full");
        for (;;);
    }

    set_frame(frame * 0x1000);
    pte->present = 1;
    pte->writeable = is_writeable ? 1 : 0;
    pte->user_access = is_user ? 1 : 0;
    pte->frame = frame;
}

// Maps a page at virtual_addr to physical_addr in RAM
void map_page(uint virtual_addr, uint physical_addr, int is_user, int is_writeable) {
    PageTableEntry *pte = get_PTE(virtual_addr, current_page_directory, 1);

    // The page is already mapped
    if (pte->frame != 0) return;

//    debug_i("Frame:", virtual_addr);

    set_frame(physical_addr);
    pte->present = 1;
    pte->writeable = is_writeable ? 1 : 0;
    pte->user_access = is_user ? 1 : 0;
    pte->frame = physical_addr / 0x1000;
}

// Unmap a page
void unmap_page(uint virtual_addr) {
    PageTableEntry *pte = get_PTE(virtual_addr, current_page_directory, 0);

    // If the page isn't mapped, nothing to do
    if (pte == 0 || pte->frame == 0) return;

    clear_frame(pte->frame * 0x1000);
    pte->frame = 0;
}

uint get_PTE_val(uint address) {
    uint *pte = (uint*)get_PTE(address, current_page_directory, 0);
    return *pte;
}

// Maps a page to forbidden_page when someone shouldn't
// have access to that page
void map_forbidden(uint virtual_addr) {
    map_page(virtual_addr, (uint)forbidden_page, 1, 0);
}

PageTableEntry *get_PTE(uint address, PageDirectory *dir, int create_if_not_exist) {
    address /= 0x1000;
    uint page_table_idx = address / 1024;
    uint page_table_entry_idx = address % 1024;

    // If the page table already exists, return the PTE
    if (dir->entry[page_table_idx])
        return &( ((PageTable *)(dir->entry[page_table_idx] & 0xFFFFF000))->pte[page_table_entry_idx] );

    // If it doesn't exist and we don't want to create one,
    // return 0
    if (!create_if_not_exist) return 0;

    // Allocate a new page table (= 1024 PTE)
    PageTable *pt = (PageTable *)kmalloc_pages(sizeof(PageTable) / 0x1000, "VM Page table");
    uint physical_address = (uint)pt;
//    debug_i("New page table: ", (uint)pt);
    memset(pt, 0, sizeof(PageTable));

    // Sets the page directory entry (flags = 0x1: present, 0x2: read/write, 0x4: user accesible)
    dir->entry[page_table_idx] = physical_address | 0x7;

    // Returns the correct PTE
    return &pt->pte[page_table_entry_idx];
}

PageDirectory *kernel_page_directory;
extern uint initial_esp;
PageTable page_tables[300];

char forbidden_page_motif[177] ="\
    _______     \
 .-\"       \"-.  \
/             \\ \
|  .--. .--.  | \
| /   | |   \\ | \
|)\\__/   \\__/(| \
/     /^\\     \\ \
\\__   '='   __/ \
   |       |    \
   \"VUUUUUV\"    \
                ";

void init_virtualmem()
{
    // The size of physical memory. For the moment we 
    // assume it is 16MB big.
    uint RAM_end_page = 0x1000000;
    int addr;

    // Determines the number of RAM pages (i.e. frames) given the
    // amount of RAM and initialized the frame bitmap
    nb_frames = RAM_end_page / 0x1000;
    frame_bitmap = (uint*)kmalloc(nb_frames / 8);
    memset(frame_bitmap, 0, nb_frames / 8);

    // Initializes the forbidden page
    forbidden_page = kmalloc_pages(1, "Forbidden page");
    for (int i=0; i<23; i++) memcpy(forbidden_page + i*176, forbidden_page_motif, 176);
    memcpy(forbidden_page + 4048, forbidden_page_motif, 48);

    // Create the kernel page directory
    kernel_page_directory = (PageDirectory*)kmalloc_pages(sizeof(PageDirectory) / 0x1000, "VM Page directory");
    memset(kernel_page_directory, 0, sizeof(PageDirectory));
    current_page_directory = kernel_page_directory;

    // Map the whole memory (right now, virtual mem = physical mem)
    // Note that we map beyond the current end of the heap
    // Because we also map future heap
    
    // Video buffer (text and VGA): user, writeable
    for (addr = 0x10000; addr < 0xC0000; addr += 0x1000) {
        map_page(addr, addr, 1, 1);
    }

    // Code and rodata: user, readonly
    for (addr = (uint)&code; addr < (uint)&data; addr += 0x1000) {
        map_page(addr, addr, 1, 0);
    }

    // data, bss: user, writeable
    for (addr = (uint)&data; addr < (uint)&debug_info; addr += 0x1000) {
        map_page(addr, addr, 1, 1);
    }

    // Kernel heap: user, writeable
    for (addr = kheap.start; addr < kheap.page_end; addr += 0x1000) {
        map_page(addr, addr, 1, 1);
    }

/*    while (addr < 0x1000000)
    {
        // Kernel code is readable but not writeable from userspace.
        map_page(addr, addr, 1, 1);
//        map_to_first_available(addr, 1, 1);
//        alloc_frame( get_page(addr, 1, kernel_page_directory), 0, 0);
        addr += 0x1000;
    }*/

    // Before we enable paging, we must register our page fault handler.
    register_interrupt_handler(14, &page_fault);
/*
    debug_i("kernel_directory: ", (uint)kernel_page_directory);
    debug_i("kernel_directory (end): ", (uint)kernel_page_directory + sizeof(page_directory));
    debug_i("Stack: ", (uint)&initial_esp);
    debug_i("End of heap: ", next_memory_block);
    debug_i("Forbidden page: ", (uint)forbidden_page);
*/
    // Now, enable paging!
    switch_page_directory(kernel_page_directory);
//    print_page_directory(kernel_page_directory);
}

void switch_page_directory(PageDirectory *dir)
{
    // Sets the new page directory (in the kernel and the CPU)
    current_page_directory = dir;
    asm volatile("mov %0, %%cr3":: "r"(dir));

    // Flips the bit in CR0 to enable paging
    uint cr0;
    asm volatile("mov %%cr0, %0": "=r"(cr0));
    cr0 |= 0x80000000;    
    asm volatile("mov %0, %%cr0":: "r"(cr0));
}

static PageTable *clone_page_table(PageTable *src, uint *physAddr)
{
    // Make a new page table, which is page aligned.
    PageTable *dst = (PageTable*)kmalloc_pages(sizeof(PageTable) / 0x1000, "VM Page table");
    *physAddr = (uint)dst;
    // Ensure that the new table is blank.
    memset(dst, 0, sizeof(PageTable));

    // For every entry in the table...
    int i;
    for (i = 0; i < 1024; i++)
    {
        // If the source entry has a frame associated with it...
        if (src->pte[i].frame)
        {
            // Get a new frame.
            map_to_first_available((uint)&dst->pte[i], 0, 0);
            // Clone the flags from source to destination.
            if (src->pte[i].present)    dst->pte[i].present = 1;
            if (src->pte[i].writeable)  dst->pte[i].writeable = 1;
            if (src->pte[i].user_access) dst->pte[i].user_access = 1;
            if (src->pte[i].accessed)   dst->pte[i].accessed = 1;
            if (src->pte[i].dirty)      dst->pte[i].dirty = 1;
            // Physically copy the data across. This function is in process.s.
            copy_physical_page(src->pte[i].frame*0x1000, dst->pte[i].frame*0x1000);
        }
    }
    return dst;
}

PageDirectory *clone_page_directory(PageDirectory *src)
{
    // Make a new page directory and obtain its physical address.
    PageDirectory *dst = (PageDirectory*)kmalloc_pages(sizeof(PageDirectory) / 0x1000, "VM Page directory");
    uint phys = (uint)dst;
    // Ensure that it is blank.
    memset(dst, 0, sizeof(PageDirectory));

    // Go through each page table. If the page table is in the kernel directory, do not make a new copy.
    int i;
    for (i = 0; i < 1024; i++)
    {
        if (!src->entry[i])
            continue;

        if (kernel_page_directory->entry[i] == src->entry[i])
        {
            // It's in the kernel, so just use the same pointer.
            dst->entry[i] = src->entry[i];
        }
        else
        {
            // Copy the table.
            uint phys;
            clone_page_table((PageTable *)(src->entry[i] & 0xFFFFF000), &phys);
            dst->entry[i] = phys | 0x07;
        }
    }

//    print_page_directory(dst);

    return dst;
}

// Debug function that prints the contents of a page directory
/*
void print_page_directory(PageDirectory *dir) {
    debug_i("Page directory at: ", (uint)dir);
    for (int i=0; i<3; i++) {
        if (dir->entry[i] == 0) continue;

        PageTable *pt = (PageTable *)(dir->entry[i] & 0xFFFFF000);
        debug("- Entry (");
        if (dir->entry[i] & 0x1) debug("present ");
        if (dir->entry[i] & 0x2) debug("writeable ");
        if (dir->entry[i] & 0x4) debug("user_access");
        debug_i(") -> ", (uint)pt);

        for (int j=0; j<4; j++) {
            PageTableEntry *pte = &(pt->pte[j]);
            if (pte->frame == 0) continue;

            debug("    - Page table entry (");
            if (pte->present) debug("present ");
            if (pte->writeable) debug("writeable ");
            if (pte->user_access) debug("user_access");
            debug_i(") -> ", pte->frame * 0x1000);
        }
    }
}
*/

#define DIR_MAPPED      1
#define DIR_WRITEABLE   2
#define DIR_USER        4

void print_page_directory_status(uint *status, uint *new_status, uint *address, uint new_address, Window *win) {
    if (*new_status != *status) {
        printf_win(win, "%x -> %x ", *address, new_address);
        if (*status & DIR_MAPPED) {
            if (*status & DIR_WRITEABLE) printf_win(win, "Read/Write "); else printf_win(win, "Read       ");
            if (*status & DIR_USER) printf_win(win, "User"); else printf_win(win, "Kernel");
        }
        else printf_win(win, "-");
        printf_win(win, "\n");

        *status = *new_status;
        *address = new_address;
    }
}

void print_page_directory(PageDirectory *dir, Window *win) {
    uint status = 0, new_status;
    uint address = 0, new_address;

    for (int i=0; i<1024; i++) {
        new_address = i * 1024 * 0x1000;

        if (dir->entry[i] == 0) {
            new_status = 0;
            print_page_directory_status(&status, &new_status, &address, new_address, win);
            continue;
        }

        PageTable *pt = (PageTable *)(dir->entry[i] & 0xFFFFF000);

        for (int j=0; j<1024; j++) {
            PageTableEntry *pte = &(pt->pte[j]);

            if (pte->frame == 0) {
                new_status = 0;
                print_page_directory_status(&status, &new_status, &address, new_address, win);
                new_address += 0x1000;
                continue;
            }

            new_status = 0;
            if (pte->present) new_status |= DIR_MAPPED;
            if (pte->writeable) new_status |= DIR_WRITEABLE;
            if (pte->user_access) new_status |= DIR_USER;
            print_page_directory_status(&status, &new_status, &address, new_address, win);

            new_address += 0x1000;
        }
    }

    new_address = 0xFFFFFFFF;
    new_status = 0xFFFFFFFF;
    print_page_directory_status(&status, &new_status, &address, new_address, win);
}

void memory_print(Window *win) {
    print_page_directory(current_page_directory, win);
}

// Handler called whenever a page fault happens, i.e. the program tried to access an address
// that is either not mapped yet or mapped to a restricted address
// Right now we handle the error gracefully by printing some debug information and mapping that
// page to the forbidden page
static void page_fault(registers_t regs)
{
    // A page fault has occurred.
    // The faulting address is stored in the CR2 register.
    uint faulting_address;
    asm volatile("mov %%cr2, %0" : "=r" (faulting_address));
    
    // The error code gives us details of what happened.
    int present   = !(regs.err_code & 0x1); // Page not present
    int rw = regs.err_code & 0x2;           // Write operation?
    int us = regs.err_code & 0x4;           // Processor was in user-mode?
    int reserved = regs.err_code & 0x8;     // Overwritten CPU-reserved bits of page entry?
    int id = regs.err_code & 0x10;          // Caused by an instruction fetch?

    // Output an error message.
    printf("Page fault! %x %x ( ", regs.esp, regs.ebp);
    if (present) {printf("present ");}
    if (rw) {printf("read-only ");}
    if (us) {printf("user-mode ");}
    if (reserved) {printf("reserved ");}
    printf(") at [%x] \n", faulting_address);

//    stack_dump();
    C_stack_dump((void*)regs.esp, (void*)regs.ebp);
//    for (;;);
    // Maps to the forbidden page
    map_forbidden(faulting_address & 0xFFFFF000);
//    map_to_first_available(faulting_address & 0xFFFFF000, 1, 1);
}
