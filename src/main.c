#include <efi.h>
#include <efilib.h>

static EFI_SYSTEM_TABLE *st = NULL;
static void console_print(const CHAR16 *s) {
    if (s == NULL) {
        return;
    }
    if (st != NULL && st->ConOut != NULL) {
        uefi_call_wrapper(st->ConOut->OutputString, 2, st->ConOut, (CHAR16 *)s);
    }
}
EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    EFI_STATUS status;

    InitializeLib(ImageHandle, SystemTable);
    st = SystemTable;

    console_print(L"Hello from GNU-EFI!\r\n");
    console_print(L"Sleeping for 10 seconds...\r\n");
    status = uefi_call_wrapper(SystemTable->BootServices->Stall, 1, 10 * 1000 * 1000);
    if (EFI_ERROR(status)) {
        return status;
    }
    console_print(L"Done. Exiting application.\r\n");

    return EFI_SUCCESS;
}