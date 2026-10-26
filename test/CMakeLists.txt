cmake_minimum_required(VERSION 3.20)
project(x64os_tests C)

enable_testing()

add_executable(test_common test_common.c ../src/common.c)
target_include_directories(test_common PRIVATE ../include)
target_compile_features(test_common PRIVATE c_std_11)
# Keep the kernel formatter separate from the host libc printf.
target_compile_definitions(test_common PRIVATE printf=kernel_printf)
target_compile_options(test_common PRIVATE -ffreestanding -Wall -Wextra -Werror)

foreach(case IN ITEMS string hex formats integers pointers fallback stack_args width)
    add_test(NAME common_${case} COMMAND test_common ${case})
endforeach()
