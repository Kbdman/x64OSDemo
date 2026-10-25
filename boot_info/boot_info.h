#ifndef BOOT_INFO_H
#define BOOT_INFO_H
#include <stdint.h>
#include <efi.h>
typedef struct
{
    void *stack;
    EFI_MEMORY_DESCRIPTOR *memory_map;
    uint64_t memory_map_size;
    uint64_t map_key;
    uint64_t descriptor_size;
    uint32_t descriptor_version;
} boot_info_t;
#endif
