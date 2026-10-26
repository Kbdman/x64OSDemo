#include <common.h>
#include <serial.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <efi.h>
const char *MemoryTypeName(EFI_MEMORY_TYPE type)
{
    switch (type)
    {
    case EfiReservedMemoryType:
        return "EfiReservedMemoryType";
    case EfiLoaderCode:
        return "EfiLoaderCode";
    case EfiLoaderData:
        return "EfiLoaderData";
    case EfiBootServicesCode:
        return "EfiBootServicesCode";
    case EfiBootServicesData:
        return "EfiBootServicesData";
    case EfiRuntimeServicesCode:
        return "EfiRuntimeServicesCode";
    case EfiRuntimeServicesData:
        return "EfiRuntimeServicesData";
    case EfiConventionalMemory:
        return "EfiConventionalMemory";
    case EfiUnusableMemory:
        return "EfiUnusableMemory";
    case EfiACPIReclaimMemory:
        return "EfiACPIReclaimMemory";
    case EfiACPIMemoryNVS:
        return "EfiACPIMemoryNVS";
    case EfiMemoryMappedIO:
        return "EfiMemoryMappedIO";
    case EfiMemoryMappedIOPortSpace:
        return "EfiMemoryMappedIOPortSpace";
    case EfiPalCode:
        return "EfiPalCode";
    case EfiPersistentMemory:
        return "EfiPersistentMemory";
    case EfiUnacceptedMemoryType:
        return "EfiUnacceptedMemoryType";
    default:
        return "Unknown";
    }
}
void print_string(const char *str)
{
    while (*str)
    {
        write_c_serial(*str++);
    }
}

void print_hex(unsigned long value)
{
    char hex[17];
    const char *digits = "0123456789ABCDEF";
    for (int i = 0; i < 16; i++)
    {
        hex[15 - i] = digits[value & 0xF];
        value >>= 4;
    }
    hex[16] = '\0';
    print_string(hex);
}

static size_t number_to_string(uintptr_t value, unsigned int base, char *buffer)
{
    const char *digits = "0123456789abcdef";
    char reverse[sizeof(uintptr_t) * 8];
    unsigned int count = 0;
    do
    {
        reverse[count++] = digits[value % base];
        value /= base;
    } while (value != 0);
    for (unsigned int i = 0; i < count; i++)
    {
        buffer[i] = reverse[count - i - 1];
    }
    return count;
}

static void write_field(const char *value, size_t length, size_t width,
                        int left_align, char padding, size_t prefix_length,
                        size_t field_length)
{
    size_t pad_count = width > field_length ? width - field_length : 0;
    if (!left_align && padding == '0' && prefix_length != 0)
    {
        for (size_t i = 0; i < prefix_length; i++)
            write_c_serial(value[i]);
        while (pad_count != 0)
        {
            write_c_serial('0');
            pad_count--;
        }
        value += prefix_length;
        length -= prefix_length;
    }
    else if (!left_align)
    {
        while (pad_count != 0)
        {
            write_c_serial(padding);
            pad_count--;
        }
    }

    for (size_t i = 0; i < length; i++)
        write_c_serial(value[i]);

    if (left_align)
    {
        while (pad_count != 0)
        {
            write_c_serial(' ');
            pad_count--;
        }
    }
}

void printf(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    while (*format != '\0')
    {
        if (*format != '%')
        {
            write_c_serial(*format++);
            continue;
        }
        format++;
        int left_align = 0;
        int zero_pad = 0;
        while (*format == '-' || *format == '0')
        {
            if (*format == '-')
                left_align = 1;
            else
                zero_pad = 1;
            format++;
        }

        size_t width = 0;
        while (*format >= '0' && *format <= '9')
        {
            size_t digit = (size_t)(*format - '0');
            if (width <= (SIZE_MAX - digit) / 10)
                width = width * 10 + digit;
            else
                width = SIZE_MAX;
            format++;
        }

        char buffer[sizeof(uintptr_t) * 8 + 2];
        const char *value = buffer;
        size_t length = 0;
        size_t prefix_length = 0;
        int numeric = 0;
        switch (*format)
        {
        case 's':
        {
            value = va_arg(args, const char *);
            if (value == NULL)
                value = "(null)";
            while (value[length] != '\0')
                length++;
            break;
        }
        case 'c':
            buffer[0] = (char)va_arg(args, int);
            length = 1;
            break;
        case 'd':
        {
            int value = va_arg(args, int);
            unsigned int magnitude = (unsigned int)value;
            if (value < 0)
            {
                buffer[0] = '-';
                magnitude = 0u - magnitude;
                prefix_length = 1;
            }
            length = prefix_length + number_to_string(magnitude, 10, buffer + prefix_length);
            numeric = 1;
            break;
        }
        case 'u':
            length = number_to_string(va_arg(args, unsigned int), 10, buffer);
            numeric = 1;
            break;
        case 'x':
            length = number_to_string(va_arg(args, unsigned int), 16, buffer);
            numeric = 1;
            break;
        case 'p':
            buffer[0] = '0';
            buffer[1] = 'x';
            length = 2 + number_to_string((uintptr_t)va_arg(args, void *), 16, buffer + 2);
            prefix_length = 2;
            numeric = 1;
            break;
        case '%':
            buffer[0] = '%';
            length = 1;
            break;
        case '\0':
            write_c_serial('%');
            va_end(args);
            return;
        default:
            write_c_serial('%');
            write_c_serial(*format);
            format++;
            continue;
        }
        write_field(value, length, width, left_align,
                    zero_pad && numeric && !left_align ? '0' : ' ',
                    prefix_length, *format == 'p' ? length - 2 : length);
        format++;
    }
    va_end(args);
}
