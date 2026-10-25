#include <common.h>
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
    printf("GDT Base: %p\tGDT Limit: %x\n",
           (void *)gdt.base, (unsigned int)gdt.limit);
    for (int i = 0; i <= gdt.limit / sizeof(segment_descriptor); i++)
    {
        segment_descriptor *seg = &gdt.base[i];
        unsigned long base = get_base(seg);
        printf("Segment Base: %p\tLimit: %x\tType: %x\n",
               (void *)base,
               (unsigned int)(seg->info & 0xFFFF),
               (unsigned int)get_type(seg));
    }
}
