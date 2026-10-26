#include <boot_info.h>
#include <common.h>
#include <serial.h>
#include <gdt.h>
void parse_boot_info(boot_info_t *bootinfo)
{
    if (bootinfo->descriptor_version != EFI_MEMORY_DESCRIPTOR_VERSION)
    {
        printf("not supported descriptor version %d,shoudl be %d\n", bootinfo->descriptor_version, EFI_MEMORY_DESCRIPTOR_VERSION);
        return;
    }
    uint64_t descriptor_size = bootinfo->descriptor_size;
    uint64_t map_size = bootinfo->memory_map_size;
    uint64_t descriptor_count = map_size / descriptor_size;
    printf("Boot info at %p\n", (void *)bootinfo);
    printf("Stack at %p\n", bootinfo->stack);
    printf("Memory map at %p\n", (void *)bootinfo->memory_map);
    printf("Memory map size: %d\n", (unsigned int)bootinfo->memory_map_size);
    printf("descriptor size: %d\n", (unsigned int)bootinfo->descriptor_size);
    printf("descriptor version: %d\n", (unsigned int)bootinfo->descriptor_version);
    printf("Number of memory map entries: %d\n", descriptor_count);
    unsigned int free_pages = 0;
    for (int i = 0; i < descriptor_count; i++)
    {
        EFI_MEMORY_DESCRIPTOR *desc =
            (EFI_MEMORY_DESCRIPTOR *)((uint8_t *)bootinfo->memory_map + i * descriptor_size);
        printf("Memory map entry %3d at %p\t", i, desc);
        printf("Physical Start: %016p, Number of Pages: %10d,Type: %s\n",
               (void *)desc->PhysicalStart, desc->NumberOfPages, MemoryTypeName(desc->Type));
        if (desc->Type == EfiConventionalMemory)
            free_pages += desc->NumberOfPages;
    }
    printf("Total free pages: %d, size: %d MB\n", free_pages, free_pages * 4096 / 1024 / 1024);
}
void entry(boot_info_t *bootinfo)
{

    init_serial();
    printf("Serial initialized\n");
    get_gdt();
    show_gdt();
    parse_boot_info(bootinfo);
    while (1)
    {
    }
}
