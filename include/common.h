#ifndef COMMON_H
#define COMMON_H
#include <efi.h>
#define PAGE_SIZE 4096
void outport(unsigned short port, unsigned char value);
unsigned char inport(unsigned short port);
void print_string(const char *str);
void print_hex(unsigned long value);
/* Serial formatter: %s, %c, %d, %u, %x, %p, %% with optional width, -, and 0 flags. */
/* Pointer width excludes the 0x prefix. */
void printf(const char *format, ...);
const char *MemoryTypeName(EFI_MEMORY_TYPE type);
uint64_t upround(uint64_t value, uint64_t alignment);
void *upround_to_page(void *address);
void memset(void *ptr, unsigned char value, uint64_t size);
#endif // COMMON_H
