#include <efi.h>
#include <efilib.h>

static inline void outb(unsigned short port, unsigned char value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static void debugcon_write_char(char c) {
    outb(0x402, (unsigned char)c);
}

static void debugcon_write_string(const char *s) {
    while (*s != '\0') {
        if (*s == '\n') {
            debugcon_write_char('\r');
        }
        debugcon_write_char(*s++);
    }
}

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    EFI_STATUS status;

    InitializeLib(ImageHandle, SystemTable);

    debugcon_write_string("[DEBUGCON] Hello from OVMF debugcon\n");
    Print(L"Hello from GNU-EFI!\r\n");
    Print(L"Sleeping for 10 seconds...\r\n");

    status = uefi_call_wrapper(SystemTable->BootServices->Stall, 1, 10 * 1000 * 1000);
    if (EFI_ERROR(status)) {
        debugcon_write_string("[DEBUGCON] Stall failed\n");
        return status;
    }

    debugcon_write_string("[DEBUGCON] Done. Exiting application.\n");
    Print(L"Done. Exiting application.\r\n");

    return EFI_SUCCESS;
}