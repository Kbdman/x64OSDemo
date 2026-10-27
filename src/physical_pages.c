#include <physical_pages.h>
#include <boot_info.h>
#include <common.h>
physical_pages_map_t *physical_pages_map = (physical_pages_map_t *)-1;
void mark_pages(uint64_t start, uint64_t count, int used)
{
    if (physical_pages_map == (physical_pages_map_t *)-1)
        return;
    if (count == 0)
        return;
    uint64_t start_page = start / PAGE_SIZE;
    if (count - 1 > UINT64_MAX - start_page ||
        (start_page + count - 1) / 8 >= physical_pages_map->mapsize)
    {
        printf("mark_pages: range exceeds physical pages bitmap\n");
        return;
    }
    for (uint64_t i = 0; i < count; i++)
    {
        uint64_t page = start_page + i;
        uint64_t byte_index = page / 8;
        uint8_t bit_index = page % 8;
        if (used)
            physical_pages_map->bitmap[byte_index] |= (1 << bit_index);
        else
            physical_pages_map->bitmap[byte_index] &= ~(1 << bit_index);
    }
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
    uint64_t bitmap_bytes = upround(total_pages, 8) / 8;
    uint64_t map_bytes = sizeof(PAGE_NUMBER_TYPE) + bitmap_bytes;
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
            break;
        }
    }
    memset(physical_pages_map->bitmap, 0xff, bitmap_bytes);
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
                mark_pages(desc->PhysicalStart, desc->NumberOfPages, 0);
            else
            {
                mark_pages(desc->PhysicalStart + needed_pages * PAGE_SIZE, desc->NumberOfPages - needed_pages, 0);
            }
        }
    }
    print_physical_pages_map();
}
