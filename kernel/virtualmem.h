#ifndef VIRTUALMEM_H
#define VIRTUALMEM_H

// The page_table_entry, page_table and page_directory structures are specific to the
// i386 family of processors

typedef struct {
  uint present          : 1;
  uint writeable        : 1;
  uint user_access      : 1;
  uint write_through    : 1;
  uint cache_disabled   : 1;
  uint accessed         : 1;
  uint dirty            : 1;
  uint pat              : 1;
  uint global_page      : 1;
  uint avail_1          : 1;
  uint avail_2          : 1;
  uint avail_3          : 1;
  uint frame            : 20;
} PageTableEntry;

typedef struct {
  PageTableEntry pte[1024];
} PageTable;

typedef struct {
  uint entry[1024];
  PageTable *tables[1024];
} PageDirectory;

void init_virtualmem();
void switch_page_directory(PageDirectory *dir);
PageTableEntry *get_PTE(uint address, PageDirectory *dir, int create_if_not_exist);
void map_page(uint virtual_addr, uint physical_addr, int is_user, int is_writeable);
PageDirectory *clone_page_directory(PageDirectory *src);

#endif
