void outport(unsigned short port, unsigned char value);
unsigned char inport(unsigned short port);
void print_string(const char *str);
void print_hex(unsigned long value);
/* Serial formatter: %s, %c, %d, %u, %x, %p, %% with optional width, -, and 0 flags. */
void printf(const char *format, ...);
