#include <boot_info.h>
#include <common.h>
#include <serial.h>
#include <gdt.h>
void parse_boot_info(boot_info_t *bootinfo)
{
}
void entry(boot_info_t *bootinfo)
{

    init_serial();
    printf("Serial initialized\n");
    get_gdt();
    show_gdt();

    while (1)
    {
    }
}
