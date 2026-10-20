#include <efi.h>
#include <efilib.h>
#define PAGE_SIZE 4096
static EFI_SYSTEM_TABLE *st = NULL;
static inline void outb(unsigned short port, unsigned char value)
{
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static void debugcon_write_char(char c)
{
    outb(0x402, (unsigned char)c);
}

static void debugcon_write_string(const char *s)
{
    while (*s != '\0')
    {
        if (*s == '\n')
        {
            debugcon_write_char('\r');
        }
        debugcon_write_char(*s++);
    }
}
void Sleep(UINTN milliseconds)
{
    EFI_STATUS status = uefi_call_wrapper(st->BootServices->Stall, 1, milliseconds * 1000);
    if (EFI_ERROR(status))
    {
        debugcon_write_string("[DEBUGCON] Stall failed\n");
    }
}
EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
    EFI_STATUS status;
    UINTN MemoryMapSize = 0;
    EFI_MEMORY_DESCRIPTOR *MemoryMap = NULL;
    UINTN MapKey = 0;
    UINTN DescriptorSize = 0;
    UINT32 DescriptorVersion = 0;

    InitializeLib(ImageHandle, SystemTable);
    st = SystemTable;

    debugcon_write_string("[DEBUGCON] Hello from OVMF debugcon\n");
    Print(L"Hello from GNU-EFI!\r\nFirmware Vendor: %s\r\n", SystemTable->FirmwareVendor);
    int pages_needed = 1;
    while (1)
    {
        if (MemoryMap != NULL)
        {
            status = uefi_call_wrapper(SystemTable->BootServices->FreePages, 2, MemoryMap, pages_needed);
            if (EFI_ERROR(status))
            {
                Print(L"Free Old Pages failed: %r\r\n", status);
                return status;
            }
        }
        Print(L"AllocatePages for %d pages\n", pages_needed);
        status = uefi_call_wrapper(SystemTable->BootServices->AllocatePages, 4, AllocateAnyPages, EfiLoaderData, pages_needed, &MemoryMap);
        if (EFI_ERROR(status))
        {
            Print(L"AllocatePages failed: %r\r\n", status);
            return status;
        }
        MemoryMapSize = PAGE_SIZE * pages_needed;
        status = uefi_call_wrapper(SystemTable->BootServices->GetMemoryMap, 5, &MemoryMapSize, MemoryMap, &MapKey, &DescriptorSize, &DescriptorVersion);
        if (EFI_ERROR(status))
        {
            Print(L"GetMemoryMap failed: %r\r\n", status);
            Sleep(500);
            pages_needed++;
            continue;
        }
        else
        {
            break;
        }
    }

    for (UINTN i = 0; i < MemoryMapSize / DescriptorSize; i++)
    {
        EFI_MEMORY_DESCRIPTOR *desc = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)MemoryMap + i * DescriptorSize);
        Print(L"Memory Descriptor %lu: Type=%u, PhysicalStart=%lx,VirtualStart=%lx, NumberOfPages=%lu\n", i, desc->Type, desc->PhysicalStart, desc->VirtualStart, desc->NumberOfPages);
        Sleep(50); // Sleep for 100 milliseconds between printing each memory descriptor
    }
    Print(L"MemoryMap allocated at: %p\n", MemoryMap);
    Print(L"MemoryMapSize: %lu\n", MemoryMapSize);
    Print(L"DescriptorNumber: %lu\n", MemoryMapSize / DescriptorSize);
    Print(L"DescriptorSize: %lu\n", DescriptorSize);
    Print(L"DescriptorVersion: %u\n", DescriptorVersion);
    Print(L"MapKey: %lu\n", MapKey);
    Print(L"Sleeping for 10 seconds...\r\n");
    Sleep(10000);

    debugcon_write_string("[DEBUGCON] Done. Exiting application.\n");
    Print(L"Done. Exiting application.\r\n");
    uefi_call_wrapper(SystemTable->BootServices->FreePages, 2, MemoryMap, pages_needed);

    return EFI_SUCCESS;
}