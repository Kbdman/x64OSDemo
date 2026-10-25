#include <boot_info.h>
#include <common.h>
#include <serial.h>
#include <gdt.h>
int inited = 0;
void entry(boot_info_t *bootinfo)
{
    if (inited == 0)
    {
        init_serial();
        get_gdt();
        show_gdt();
        inited = 1;
    }
    while (1)
    {
    }
}
