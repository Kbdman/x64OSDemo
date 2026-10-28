cmake_minimum_required(VERSION 3.20)
project(x64os_tests C)

enable_testing()

find_path(EFI_INCLUDE_DIR efi.h PATH_SUFFIXES efi REQUIRED)
find_path(EFI_ARCH_INCLUDE_DIR efibind.h
    HINTS "${EFI_INCLUDE_DIR}" PATH_SUFFIXES x86_64 REQUIRED)

add_executable(test_common test_common.c ../src/common.c)
set_source_files_properties(../src/common.c PROPERTIES
    COMPILE_DEFINITIONS memset=kernel_memset)
target_include_directories(test_common PRIVATE
    ../include "${EFI_INCLUDE_DIR}" "${EFI_ARCH_INCLUDE_DIR}")
target_compile_features(test_common PRIVATE c_std_11)
# Keep the kernel formatter separate from the host libc printf.
target_compile_definitions(test_common PRIVATE printf=kernel_printf)
target_compile_options(test_common PRIVATE -ffreestanding -Wall -Wextra -Werror)

foreach(case IN ITEMS string hex formats integers pointers fallback stack_args width
                      upround upround_to_page memset)
    add_test(NAME common_${case} COMMAND test_common ${case})
endforeach()

add_executable(test_physical_pages test_physical_pages.c
    ../src/physical_pages.c ../src/common.c)
set_source_files_properties(../src/physical_pages.c PROPERTIES
    COMPILE_DEFINITIONS memset=kernel_memset)
target_include_directories(test_physical_pages PRIVATE
    ../include ../boot_info "${EFI_INCLUDE_DIR}" "${EFI_ARCH_INCLUDE_DIR}")
target_compile_features(test_physical_pages PRIVATE c_std_11)
target_compile_definitions(test_physical_pages PRIVATE printf=kernel_printf)
target_compile_options(test_physical_pages PRIVATE -ffreestanding -Wall -Wextra -Werror)

foreach(case IN ITEMS uninitialized mark clear zero_count allocate
                      allocate_fragmented allocate_bounds allocate_at
                      allocate_invalid_map bounds print)
    add_test(NAME physical_pages_${case} COMMAND test_physical_pages ${case})
endforeach()
