#include <common.h>
#include <serial.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

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

static void print_number(uintptr_t value, unsigned int base)
{
    const char *digits = "0123456789abcdef";
    char buffer[sizeof(uintptr_t) * 8];
    unsigned int count = 0;
    do
    {
        buffer[count++] = digits[value % base];
        value /= base;
    } while (value != 0);
    while (count != 0)
    {
        write_c_serial(buffer[--count]);
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
        switch (*format)
        {
        case 's':
        {
            const char *str = va_arg(args, const char *);
            print_string(str != NULL ? str : "(null)");
            break;
        }
        case 'c':
            write_c_serial((char)va_arg(args, int));
            break;
        case 'd':
        {
            int value = va_arg(args, int);
            unsigned int magnitude = (unsigned int)value;
            if (value < 0)
            {
                write_c_serial('-');
                magnitude = 0u - magnitude;
            }
            print_number(magnitude, 10);
            break;
        }
        case 'u':
            print_number(va_arg(args, unsigned int), 10);
            break;
        case 'x':
            print_number(va_arg(args, unsigned int), 16);
            break;
        case 'p':
            print_string("0x");
            print_number((uintptr_t)va_arg(args, void *), 16);
            break;
        case '%':
            write_c_serial('%');
            break;
        case '\0':
            write_c_serial('%');
            va_end(args);
            return;
        default:
            write_c_serial('%');
            write_c_serial(*format);
            break;
        }
        format++;
    }
    va_end(args);
}
