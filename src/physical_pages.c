#include <physical_pages.h>
#include <boot_info.h>
#include <common.h>
#define BIT_FOR_ONEPAGE 2
#define PAGES_PER_BYTE (8 / BIT_FOR_ONEPAGE)
#define BIT_MASK ((1 << BIT_FOR_ONEPAGE) - 1)

physical_pages_map_t *physical_pages_map = (physical_pages_map_t *)-1;

static int physical_pages_map_is_valid(void)
{
    if (physical_pages_map == (physical_pages_map_t *)-1)
        return 0;
    uint64_t required_bytes =
        physical_pages_map->total_pages / PAGES_PER_BYTE;

    if (physical_pages_map->total_pages % PAGES_PER_BYTE != 0)
        required_bytes++;
    return physical_pages_map->mapsize >= required_bytes;
}

void mark_pages(uint64_t start, uint64_t count, page_status status)
{
    if (!physical_pages_map_is_valid())
        return;
    if (status >= PAGE_UNDEFINED || status < PAGE_FREE)
        return;
    if (count == 0)
        return;
    if (start % PAGE_SIZE != 0)
        return;
    uint64_t start_page = start / PAGE_SIZE;
    if (start_page >= physical_pages_map->total_pages ||
        count > physical_pages_map->total_pages - start_page)
    {
        printf("mark_pages: range exceeds physical pages bitmap\n");
        return;
    }
    for (uint64_t i = 0; i < count; i++)
    {
        uint64_t page = start_page + i;
        uint64_t byte_index = page / PAGES_PER_BYTE;
        uint8_t bit_index = (page % PAGES_PER_BYTE) * BIT_FOR_ONEPAGE;
        uint8_t *byte = &physical_pages_map->bitmap[byte_index];
        *byte &= ~(BIT_MASK << bit_index);
        *byte |= (status << bit_index);
    }
}
void *allocate_page(uint64_t count)
{
    if (!physical_pages_map_is_valid() || count == 0 ||
        count > physical_pages_map->total_pages)
        return NULL;
    for (uint64_t i = 1; i < physical_pages_map->total_pages; i++)
    {
        uint64_t byte_index = i / PAGES_PER_BYTE;
        uint8_t bit_index = (i % PAGES_PER_BYTE) * BIT_FOR_ONEPAGE;
        if ((physical_pages_map->bitmap[byte_index] & (BIT_MASK << bit_index)) == 0)
        {
            uint64_t start_page = i;
            if (count > physical_pages_map->total_pages - start_page)
                return NULL;
            int all_free = 1;
            for (uint64_t offset = 0; offset < count; offset++)
            {
                uint64_t j = start_page + offset;
                uint64_t byte_index_j = j / PAGES_PER_BYTE;
                uint8_t bit_index_j = (j % PAGES_PER_BYTE) * BIT_FOR_ONEPAGE;
                if ((physical_pages_map->bitmap[byte_index_j] & (BIT_MASK << bit_index_j)) != 0)
                {
                    all_free = 0;
                    i = j; // skip to the next unchecked page after the used one
                    break;
                }
            }
            if (all_free)
            {
                mark_pages(start_page * PAGE_SIZE, count, PAGE_USED);
                return (void *)(start_page * PAGE_SIZE);
            }
        }
    }
    return NULL;
}
void *allocate_page_at(uint64_t start, uint64_t count)
{
    if (!physical_pages_map_is_valid() || count == 0 ||
        start % PAGE_SIZE != 0)
        return NULL;
    uint64_t start_page = start / PAGE_SIZE;
    if (start_page == 0 || start_page >= physical_pages_map->total_pages ||
        count > physical_pages_map->total_pages - start_page)
        return NULL;
    uint8_t all_free = 1;
    for (uint64_t offset = 0; offset < count; offset++)
    {
        uint64_t i = start_page + offset;
        uint64_t byte_index_i = i / PAGES_PER_BYTE;
        uint8_t bit_index_i = (i % PAGES_PER_BYTE) * BIT_FOR_ONEPAGE;
        if ((physical_pages_map->bitmap[byte_index_i] & (BIT_MASK << bit_index_i)) != 0)
        {
            all_free = 0;
            break;
        }
    }
    if (all_free)
    {
        mark_pages(start_page * PAGE_SIZE, count, PAGE_USED);
        return (void *)(start_page * PAGE_SIZE);
    }
    return NULL;
}
void print_physical_pages_map()
{
    if (physical_pages_map == (physical_pages_map_t *)-1)
        return;
    uint64_t total_bytes = physical_pages_map->mapsize;
    printf("Physical pages map at %p, mapsize: %lu bytes\n", (void *)physical_pages_map, total_bytes);
    int column_in_line = 32;
    for (uint64_t i = 0; i * column_in_line < total_bytes; i++)
    {
        for (uint8_t j = 0; j < column_in_line; j++)
        {
            if (i * column_in_line + j >= total_bytes)
                break;
            printf("%02x ", (physical_pages_map->bitmap[i * column_in_line + j]));
        }
        printf("\n");
    }
}
void init_physical_pages_map(boot_info_t *bootinfo)
{
    uint64_t descriptor_size = bootinfo->descriptor_size;
    uint64_t map_size = bootinfo->memory_map_size;
    uint64_t descriptor_count = map_size / descriptor_size;
    printf("Memory map at %p\n", (void *)bootinfo->memory_map);
    printf("Memory map size: %d\n", (unsigned int)bootinfo->memory_map_size);
    printf("descriptor size: %d\n", (unsigned int)bootinfo->descriptor_size);
    printf("descriptor version: %d\n", (unsigned int)bootinfo->descriptor_version);
    printf("Number of memory map entries: %d\n", descriptor_count);
    unsigned int free_pages = 0;
    uint64_t max_physical_address = 0;
    for (uint64_t i = 0; i < descriptor_count; i++)
    {
        EFI_MEMORY_DESCRIPTOR *desc =
            (EFI_MEMORY_DESCRIPTOR *)((uint8_t *)bootinfo->memory_map + i * descriptor_size);
        printf("Memory map entry %3d at %p\t", (int)i, desc);
        printf("Physical Start: %016p, Number of Pages: %10d,Type: %s\n",
               (void *)desc->PhysicalStart, desc->NumberOfPages, MemoryTypeName(desc->Type));
        if (desc->Type == EfiConventionalMemory)
        {
            free_pages += desc->NumberOfPages;
        }
        if (desc->Type != EfiReservedMemoryType && desc->Type != EfiMemoryMappedIO)
        {
            uint64_t end_address = desc->PhysicalStart + desc->NumberOfPages * 4096;
            if (end_address > max_physical_address)
                max_physical_address = end_address;
        }
    }
    printf("Total free pages: %d, size: %d MB\n", free_pages, free_pages * 4096 / 1024 / 1024);
    uint64_t total_pages = max_physical_address / PAGE_SIZE;
    uint64_t bitmap_bytes = upround(total_pages, PAGES_PER_BYTE) / PAGES_PER_BYTE;
    uint64_t map_bytes = sizeof(PAGE_NUMBER_TYPE) * 2 + bitmap_bytes;
    uint64_t needed_pages = upround(map_bytes, PAGE_SIZE) / PAGE_SIZE;
    printf("Max physical address: %016p , total page number: %d, need %d pages for bitmap\n", (void *)max_physical_address, total_pages, needed_pages);
    printf("Look for pages for bitmap\n");

    for (uint64_t i = 0; i < descriptor_count; i++)
    {
        EFI_MEMORY_DESCRIPTOR *desc =
            (EFI_MEMORY_DESCRIPTOR *)((uint8_t *)bootinfo->memory_map + i * descriptor_size);
        printf("Memory map entry %3d at %p\t", (int)i, desc);
        printf("Physical Start: %016p, Number of Pages: %10d,Type: %s\n",
               (void *)desc->PhysicalStart, desc->NumberOfPages, MemoryTypeName(desc->Type));
        if ((desc->Type == EfiConventionalMemory || desc->Type == EfiBootServicesCode || desc->Type == EfiBootServicesData) && desc->NumberOfPages >= needed_pages)
        {
            printf("Found suitable memory for bitmap at %p, number of pages: %d ,need:%d\n",
                   (void *)desc->PhysicalStart, desc->NumberOfPages, needed_pages);
            physical_pages_map = (physical_pages_map_t *)desc->PhysicalStart;
            physical_pages_map->mapsize = bitmap_bytes;
            physical_pages_map->total_pages = total_pages;
            break;
        }
    }
    if (physical_pages_map == (physical_pages_map_t *)-1)
    {
        printf("Failed to find suitable memory for bitmap\n");
        return;
    }
    // Initialize the bitmap with 0xaa pattern to mark all pages as reserved initially
    memset(physical_pages_map->bitmap, 0xaa, bitmap_bytes);
    for (uint64_t i = 0; i < descriptor_count; i++)
    {
        EFI_MEMORY_DESCRIPTOR *desc =
            (EFI_MEMORY_DESCRIPTOR *)((uint8_t *)bootinfo->memory_map + i * descriptor_size);
        printf("Memory map entry %3d at %p\t", (int)i, desc);
        printf("Physical Start: %016p, Number of Pages: %10d,Type: %s\n",
               (void *)desc->PhysicalStart, desc->NumberOfPages, MemoryTypeName(desc->Type));
        if (desc->Type == EfiConventionalMemory || desc->Type == EfiBootServicesCode || desc->Type == EfiBootServicesData)
        {
            if (desc->PhysicalStart != (uint64_t)physical_pages_map)
                mark_pages(desc->PhysicalStart, desc->NumberOfPages, PAGE_FREE);
            else
            {
                mark_pages(desc->PhysicalStart, needed_pages, PAGE_USED);
                mark_pages(desc->PhysicalStart + needed_pages * PAGE_SIZE, desc->NumberOfPages - needed_pages, PAGE_FREE);
            }
        }
    }
    print_physical_pages_map();
}
