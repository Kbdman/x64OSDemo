#include "common.h"
// wellknown I/O port addresses for COM1 serial port (0x3F8)
static int is_transmit_empty(void)
{
    return inport(0x3F8 + 5) & 0x20;
}

void write_c_serial(char c)
{
    while (!is_transmit_empty())
    {
    }
    outport(0x3F8, c);
}

void init_serial()
{
    outport(0x3F8 + 1, 0x00); // Disable all interrupts
    outport(0x3F8 + 3, 0x80); // Enable DLAB (set baud rate divisor)
    outport(0x3F8 + 0, 0x03); // Set divisor to 3 (lo byte) 38400 baud
    outport(0x3F8 + 1, 0x00); //                  (hi byte)
    outport(0x3F8 + 3, 0x03); // 8 bits, no parity, one stop bit
    outport(0x3F8 + 2, 0xC7); // Enable FIFO, clear them, with 14-byte threshold
    outport(0x3F8 + 4, 0x0B); // IRQs enabled, RTS/DSR set
}
