#ifndef PHYSICAL_PAGES_H
#define PHYSICAL_PAGES_H

#include <stdint.h>
#include <boot_info.h>
#define PAGE_NUMBER_TYPE uint64_t
typedef enum
{
    PAGE_FREE = 0,
    PAGE_USED = 1,
    PAGE_RESERVED = 2,
    PAGE_UNDEFINED = 3
} page_status;
typedef struct __attribute__((packed))
{
    PAGE_NUMBER_TYPE total_pages; // total number of physical pages
    PAGE_NUMBER_TYPE mapsize;     // size of the bitmap in bytes
    uint8_t bitmap[];             // bitmap for tracking used physical pages, 0 for free, 1 for used
} physical_pages_map_t;
void init_physical_pages_map(boot_info_t *bootinfo);
void mark_pages(uint64_t start, uint64_t count, page_status status);
void *allocate_page(uint64_t count);
void *allocate_page_at(uint64_t start, uint64_t count);
void print_physical_pages_map();
#endif // PHYSICAL_PAGES_H
