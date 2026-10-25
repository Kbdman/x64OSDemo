#include <boot_info.h>
#include <common.h>
#include <serial.h>
#include <gdt.h>
void parse_boot_info(boot_info_t *bootinfo)
{
    printf("Boot info at %p\n", (void *)bootinfo);
    printf("Stack at %p\n", bootinfo->stack);
    printf("Memory map at %p\n", (void *)bootinfo->memory_map);
    printf("Memory map size: %d\n", (unsigned int)bootinfo->memory_map_size);
    printf("descriptor size: %d\n", (unsigned int)bootinfo->descriptor_size);
    printf("descriptor version: %d\n", (unsigned int)bootinfo->descriptor_version);
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
