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

static const CHAR16 *MemoryTypeName(EFI_MEMORY_TYPE type)
{
    switch (type)
    {
    case EfiReservedMemoryType:
        return L"EfiReservedMemoryType";
    case EfiLoaderCode:
        return L"EfiLoaderCode";
    case EfiLoaderData:
        return L"EfiLoaderData";
    case EfiBootServicesCode:
        return L"EfiBootServicesCode";
    case EfiBootServicesData:
        return L"EfiBootServicesData";
    case EfiRuntimeServicesCode:
        return L"EfiRuntimeServicesCode";
    case EfiRuntimeServicesData:
        return L"EfiRuntimeServicesData";
    case EfiConventionalMemory:
        return L"EfiConventionalMemory";
    case EfiUnusableMemory:
        return L"EfiUnusableMemory";
    case EfiACPIReclaimMemory:
        return L"EfiACPIReclaimMemory";
    case EfiACPIMemoryNVS:
        return L"EfiACPIMemoryNVS";
    case EfiMemoryMappedIO:
        return L"EfiMemoryMappedIO";
    case EfiMemoryMappedIOPortSpace:
        return L"EfiMemoryMappedIOPortSpace";
    case EfiPalCode:
        return L"EfiPalCode";
    case EfiPersistentMemory:
        return L"EfiPersistentMemory";
    case EfiUnacceptedMemoryType:
        return L"EfiUnacceptedMemoryType";
    default:
        return L"Unknown";
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
            status = uefi_call_wrapper(SystemTable->BootServices->FreePages, 2, (EFI_PHYSICAL_ADDRESS)(UINTN)MemoryMap, pages_needed);
            if (EFI_ERROR(status))
            {
                Print(L"Free Old Pages failed: %r\r\n", status);
                return status;
            }
        }

        Print(L"AllocatePages for %d pages (type: %s)\r\n", pages_needed, MemoryTypeName(EfiLoaderData));
        status = uefi_call_wrapper(SystemTable->BootServices->AllocatePages, 4, AllocateAnyPages, EfiLoaderData, pages_needed, (EFI_PHYSICAL_ADDRESS *)&MemoryMap);
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

    uint64_t free_pages = 0;
    for (UINTN i = 0; i < MemoryMapSize / DescriptorSize; i++)
    {
        EFI_MEMORY_DESCRIPTOR *desc = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)MemoryMap + i * DescriptorSize);
        Print(L"Memory Descriptor %lu: Type=%s, PhysicalStart=0x%lx, VirtualStart=0x%lx, NumberOfPages=%lu\r\n",
              i,
              MemoryTypeName(desc->Type),
              desc->PhysicalStart,
              desc->VirtualStart,
              desc->NumberOfPages);
        if (desc->Type == EfiConventionalMemory)
        {
            free_pages += desc->NumberOfPages;
        }
    }

    Print(L"MemoryMap allocated at: %p\r\n", MemoryMap);
    Print(L"MemoryMapSize: %lu\r\n", MemoryMapSize);
    Print(L"DescriptorNumber: %lu\r\n", MemoryMapSize / DescriptorSize);
    Print(L"DescriptorSize: %lu\r\n", DescriptorSize);
    Print(L"DescriptorVersion: %u\r\n", DescriptorVersion);
    Print(L"MapKey: %lu\r\n", MapKey);
    Print(L"Free Memory Size: %lu MB\r\n", free_pages * PAGE_SIZE / (1024 * 1024));
    Print(L"Sleeping for 10 seconds...\r\n");

    EFI_DEVICE_PATH_TO_TEXT_PROTOCOL *DevicePathToText = NULL;

    Print(L"Wait 1s\r\n");
    Sleep(1000);
    {

        CHAR16 *VariableName = NULL;
        status = uefi_call_wrapper(SystemTable->BootServices->AllocatePages, 4, AllocateAnyPages, EfiLoaderData, 1, &VariableName);
        if (EFI_ERROR(status))
        {
            Print(L"AllocatePages failed: %r\r\n", status);
            return status;
        }
        VariableName[0] = L'\0';
        UINTN VariableNameSize = 4096;
        EFI_GUID VendorGuid = {0};
        while (1)
        {
            VariableNameSize = 4096;
            status = uefi_call_wrapper(SystemTable->RuntimeServices->GetNextVariableName, 3, &VariableNameSize, VariableName, &VendorGuid);
            if (EFI_ERROR(status))
            {
                if (status != EFI_NOT_FOUND)
                    Print(L"GetNextVariableName failed: %r\r\n", status);
                break;
            }
            Print(L"VariableName: %s\r\n", VariableName);
        }
        status = uefi_call_wrapper(SystemTable->BootServices->FreePages, 2, (EFI_PHYSICAL_ADDRESS)(UINTN)VariableName, 1);
        if (EFI_ERROR(status))
        {
            Print(L"FreePages for VariableName failed: %r\r\n", status);
            return status;
        }
        Sleep(1000);

        status = uefi_call_wrapper(SystemTable->BootServices->FreePages, 2, (EFI_PHYSICAL_ADDRESS)(UINTN)MemoryMap, pages_needed);
        if (EFI_ERROR(status))
        {
            Print(L"FreePages failed: %r\r\n", status);
            return status;
        }
    }
    Sleep(3000);
    {
        EFI_LOADED_IMAGE *LoadedImage = NULL;
        status = uefi_call_wrapper(SystemTable->BootServices->HandleProtocol, 3, ImageHandle, &gEfiLoadedImageProtocolGuid, (VOID **)&LoadedImage);

        if (EFI_ERROR(status))
        {
            Print(L"HandleProtocol failed: %r\r\n", status);
            return status;
        }
        Print(L"LoadedImage: %p\r\n", LoadedImage);
        Print(L"LoadedImage->ImageBase: %p\r\n", LoadedImage->ImageBase);
        Print(L"LoadedImage->ImageSize: %lu\r\n", LoadedImage->ImageSize);
        EFI_DEVICE_PATH *FilePath = LoadedImage->FilePath;
        Print(L"LoadedImage->FilePath SubType: %u\r\n", FilePath->SubType);
        Print(L"LoadedImage->FilePath Type: %u\r\n", FilePath->Type);
    }
    debugcon_write_string("[DEBUGCON] Done. Exiting application.\n");
    Print(L"Done. Exiting application.\r\n");
    Sleep(10000);

    return EFI_SUCCESS;
}