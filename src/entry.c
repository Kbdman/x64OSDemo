#include <boot_info.h>
#include <common.h>
#include <serial.h>
#include <gdt.h>
#include <physical_pages.h>
#include <efi.h>
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
    init_physical_pages_map(bootinfo);
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
