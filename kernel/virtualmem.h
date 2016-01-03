#ifndef PAGING_H
#define PAGING_H

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
} page_table_entry;

typedef struct {
  page_table_entry pte[1024];
} page_table;

typedef struct {
  uint entry[1024];
  page_table *tables[1024];
} page_directory;

void init_virtualmem();
void switch_page_directory(page_directory *dir);
page_table_entry *get_PTE(uint address, page_directory *dir, int create_if_not_exist);
void map_page(uint virtual_addr, uint physical_addr, int is_user, int is_writeable);
page_directory *clone_page_directory(page_directory *src);

#endif
