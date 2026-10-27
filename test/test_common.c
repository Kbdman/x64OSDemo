#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>
/* The kernel memset has a different signature from libc memset. */
#define memset kernel_memset
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

static void test_width(void)
{
    printf("|%5d|%-5d|%05d|%08x|%8s|%-6s|%5c|%5%|",
           42, 42, -42, 0x2au, "x", "xy", 'Z');
    expect_output("|   42|42   |-0042|0000002a|       x|xy    |    Z|    %|");
    printf("%020p", (void *)0);
    expect_output("0x00000000000000000000");
    printf("|%5p|%-5p|%05p|", (void *)(uintptr_t)0x2a,
           (void *)(uintptr_t)0x2a, (void *)(uintptr_t)0x2a);
    expect_output("|   0x2a|0x2a   |0x0002a|");
    printf("|%1p|%02p|%016p|", (void *)(uintptr_t)0x123,
           (void *)0, (void *)(uintptr_t)UINT64_MAX);
    expect_output("|0x123|0x00|0xffffffffffffffff|");
}

static void test_upround(void)
{
    static const struct
    {
        uint64_t value;
        uint64_t alignment;
        uint64_t expected;
    } cases[] = {
        {0, 8, 0},
        {1, 8, 8},
        {7, 8, 8},
        {8, 8, 8},
        {9, 8, 16},
        {5, 3, 6},
        {6, 3, 6},
        {21, 10, 30},
        {0, 1, 0},
        {UINT64_MAX, 1, UINT64_MAX},
        {UINT64_C(0x100000001), 8, UINT64_C(0x100000008)},
        {UINT64_MAX - 8, 8, UINT64_MAX - 7},
        {UINT64_MAX - 7, 8, UINT64_MAX - 7},
    };

    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++)
        assert(upround(cases[i].value, cases[i].alignment) == cases[i].expected);
}

static void test_upround_to_page(void)
{
    static const struct
    {
        uintptr_t address;
        uintptr_t expected;
    } cases[] = {
        {0, 0},
        {1, PAGE_SIZE},
        {PAGE_SIZE - 1, PAGE_SIZE},
        {PAGE_SIZE, PAGE_SIZE},
        {PAGE_SIZE + 1, 2 * PAGE_SIZE},
        {2 * PAGE_SIZE - 1, 2 * PAGE_SIZE},
        {2 * PAGE_SIZE, 2 * PAGE_SIZE},
        {UINT64_C(0x100000001), UINT64_C(0x100001000)},
        {UINTPTR_MAX - PAGE_SIZE, UINTPTR_MAX - (PAGE_SIZE - 1)},
        {UINTPTR_MAX - (PAGE_SIZE - 1), UINTPTR_MAX - (PAGE_SIZE - 1)},
    };

    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++)
    {
        void *result = upround_to_page((void *)cases[i].address);
        assert((uintptr_t)result == cases[i].expected);
    }
}

static void test_memset(void)
{
    unsigned char buffer[PAGE_SIZE + 3];
    static const unsigned char values[] = {0, 0x55, 0x80, 0xff};
    static const size_t lengths[] = {0, 1, 7, 8, 9, PAGE_SIZE, PAGE_SIZE + 1};

    for (size_t v = 0; v < sizeof(values) / sizeof(values[0]); v++)
    {
        for (size_t n = 0; n < sizeof(lengths) / sizeof(lengths[0]); n++)
        {
            for (size_t i = 0; i < sizeof(buffer); i++)
                buffer[i] = 0xa5;

            memset(buffer + 1, values[v], lengths[n]);

            for (size_t i = 0; i < sizeof(buffer); i++)
            {
                unsigned char expected =
                    i >= 1 && i - 1 < lengths[n] ? values[v] : 0xa5;
                assert(buffer[i] == expected);
            }
        }
    }

    memset(buffer, 0x42, sizeof(buffer));
    for (size_t i = 0; i < sizeof(buffer); i++)
        assert(buffer[i] == 0x42);

    memset(buffer + sizeof(buffer), 0xff, 0);
    for (size_t i = 0; i < sizeof(buffer); i++)
        assert(buffer[i] == 0x42);
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
    else if (strcmp(argv[1], "width") == 0)
        test_width();
    else if (strcmp(argv[1], "upround") == 0)
        test_upround();
    else if (strcmp(argv[1], "upround_to_page") == 0)
        test_upround_to_page();
    else if (strcmp(argv[1], "memset") == 0)
        test_memset();
    else
        return 1;
    return 0;
}
