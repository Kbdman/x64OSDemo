#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>
#include <common.h>
#include <serial.h>

static char output[512];
static unsigned int used;

void write_c_serial(char c)
{
    assert(used + 1 < sizeof(output));
    output[used++] = c;
    output[used] = '\0';
}

static void expect_output(const char *expected)
{
    assert(strcmp(output, expected) == 0);
    used = 0;
    output[0] = '\0';
}

static void test_string(void)
{
    print_string("");
    expect_output("");
    print_string("hello\n");
    print_string("kernel");
    expect_output("hello\nkernel");
}

static void test_hex(void)
{
    print_hex(0);
    expect_output("0000000000000000");
    print_hex(0xabcdef);
    expect_output("0000000000ABCDEF");
    print_hex(ULONG_MAX);
    expect_output("FFFFFFFFFFFFFFFF");
}

static void test_formats(void)
{
    printf("");
    expect_output("");
    printf("plain text");
    expect_output("plain text");
    printf("%s %c %d %u %x %%", "kernel", 'A', -42, 42u, 0xabu);
    expect_output("kernel A -42 42 ab %");
    printf("%s", (const char *)0);
    expect_output("(null)");
}

static void test_integers(void)
{
    printf("%d %d %d %u %u %x", INT_MIN, INT_MAX, 0, 0u, UINT_MAX, UINT_MAX);
    expect_output("-2147483648 2147483647 0 0 4294967295 ffffffff");
    printf("%x", 0u);
    expect_output("0");
}

static void test_pointers(void)
{
    printf("%p", (void *)0);
    expect_output("0x0");
    printf("%p", (void *)(uintptr_t)0x123456789abcdef0ULL);
    expect_output("0x123456789abcdef0");
}

static void test_fallback(void)
{
    printf("%q end%");
    expect_output("%q end%");
}

static void test_stack_args(void)
{
    printf("%d%d%d%d%d%d%d%d", 1, 2, 3, 4, 5, 6, 7, 8);
    expect_output("12345678");
}

int main(int argc, char **argv)
{
    _Static_assert(sizeof(unsigned long) == 8, "Tests require a 64-bit Linux host");
    assert(argc == 2);
    if (strcmp(argv[1], "string") == 0)
        test_string();
    else if (strcmp(argv[1], "hex") == 0)
        test_hex();
    else if (strcmp(argv[1], "formats") == 0)
        test_formats();
    else if (strcmp(argv[1], "integers") == 0)
        test_integers();
    else if (strcmp(argv[1], "pointers") == 0)
        test_pointers();
    else if (strcmp(argv[1], "fallback") == 0)
        test_fallback();
    else if (strcmp(argv[1], "stack_args") == 0)
        test_stack_args();
    else
        return 1;
    return 0;
}
