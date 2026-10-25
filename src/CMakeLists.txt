cmake_minimum_required(VERSION 3.20)
project(x64os_kernel C ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

file(GLOB KERNEL_C_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.c")
file(GLOB KERNEL_ASM_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.s")

add_executable(kernel.elf
    ${KERNEL_C_SOURCES}
    ${KERNEL_ASM_SOURCES}
)

target_compile_options(kernel.elf PRIVATE
    -g
    -O0
    -ffreestanding
    -fno-stack-protector
    -fno-pie
    -mno-red-zone
)
target_include_directories(kernel.elf PRIVATE
                    "${CMAKE_CURRENT_SOURCE_DIR}/../boot_info"
                    "${CMAKE_CURRENT_SOURCE_DIR}/../include"
)
target_link_options(kernel.elf PRIVATE
    -no-pie
    -nostdlib
    -T${CMAKE_CURRENT_SOURCE_DIR}/kernel.ld
)

set_property(TARGET kernel.elf APPEND PROPERTY
    LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/kernel.ld"
)

set(KERNEL_BIN "${CMAKE_CURRENT_BINARY_DIR}/kernel.bin")

add_custom_command(
    OUTPUT "${KERNEL_BIN}"
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:kernel.elf> "${KERNEL_BIN}"
    DEPENDS kernel.elf
    VERBATIM
)

add_custom_target(kernel_bin ALL DEPENDS "${KERNEL_BIN}")
