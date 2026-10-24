#include "serial.h"
typedef struct __attribute__((packed))
{
    unsigned long long info;
} segment_descriptor;
unsigned char get_type(segment_descriptor *seg)
{
    return (seg->info >> 40) & 0xF;
}
unsigned long get_base(segment_descriptor *seg)
{
    return (seg->info >> 16) & 0xFFFFFFFF | ((seg->info >> 32) & 0xFFFFFFFF);
}
typedef struct __attribute__((packed))
{
    unsigned short limit;
    segment_descriptor *base;
} gdt_descriptor_table;

gdt_descriptor_table gdt;
void get_gdt()
{
    asm volatile("sgdt %0" : "=m"(gdt));
}
void show_gdt()
{
    get_gdt();
    write_string_serial("GDT Base: ");
    write_hex_serial((unsigned long)gdt.base);
    write_string_serial("\t");
    write_string_serial("GDT Limit: ");
    write_hex_serial((unsigned long)gdt.limit);
    write_string_serial("\n");
    for (int i = 0; i <= gdt.limit / sizeof(segment_descriptor); i++)
    {
        segment_descriptor *seg = &gdt.base[i];
        unsigned long base = get_base(seg);
        write_string_serial("Segment Base: ");
        write_hex_serial(base);
        write_string_serial("\t");
        write_string_serial("Limit: ");
        write_hex_serial((unsigned long)(seg->info & 0xFFFF));
        write_string_serial("\tType: ");
        write_hex_serial((unsigned long)get_type(seg));
        write_string_serial("\n");
    }
}
