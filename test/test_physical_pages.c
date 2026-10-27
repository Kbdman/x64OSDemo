#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#define memset kernel_memset
#include <common.h>
#include <physical_pages.h>

extern physical_pages_map_t *physical_pages_map;
void print_physical_pages_map(void);

static char output[4096];
static size_t used;

void write_c_serial(char c)
{
    assert(used + 1 < sizeof(output));
    output[used++] = c;
    output[used] = '\0';
}

static void setup_map(size_t bytes, unsigned char value)
{
    physical_pages_map = malloc(sizeof(*physical_pages_map) + bytes + 1);
    assert(physical_pages_map != NULL);
    physical_pages_map->mapsize = bytes;
    for (size_t i = 0; i < bytes; i++)
        physical_pages_map->bitmap[i] = value;
    physical_pages_map->bitmap[bytes] = 0xa5;
}

static void expect_bitmap(unsigned char first, unsigned char second)
{
    assert(physical_pages_map->mapsize == 2);
    assert(physical_pages_map->bitmap[0] == first);
    assert(physical_pages_map->bitmap[1] == second);
    assert(physical_pages_map->bitmap[2] == 0xa5);
}

static void test_uninitialized(void)
{
    mark_pages(0, 1, 1);
    print_physical_pages_map();
    assert(used == 0);
    assert(physical_pages_map == (physical_pages_map_t *)-1);
}

static void test_mark(void)
{
    setup_map(2, 0);
    mark_pages(0, 1, 1);
    expect_bitmap(0x01, 0x00);
    mark_pages(6 * PAGE_SIZE, 4, 1);
    expect_bitmap(0xc1, 0x03);
    mark_pages(6 * PAGE_SIZE, 4, 1);
    expect_bitmap(0xc1, 0x03);
    mark_pages(15 * PAGE_SIZE, 1, 2);
    expect_bitmap(0xc1, 0x83);
    mark_pages(0, 16, 1);
    expect_bitmap(0xff, 0xff);
}

static void test_clear(void)
{
    setup_map(2, 0xff);
    mark_pages(6 * PAGE_SIZE, 4, 0);
    expect_bitmap(0x3f, 0xfc);
    mark_pages(6 * PAGE_SIZE, 4, 0);
    expect_bitmap(0x3f, 0xfc);
    mark_pages(15 * PAGE_SIZE, 1, 0);
    expect_bitmap(0x3f, 0x7c);
    mark_pages(0, 16, 0);
    expect_bitmap(0, 0);
}

static void test_zero_count(void)
{
    setup_map(2, 0x5a);
    mark_pages(0, 0, 1);
    mark_pages(16 * PAGE_SIZE, 0, 0);
    expect_bitmap(0x5a, 0x5a);
}

static void test_bounds(void)
{
    setup_map(2, 0x5a);
    mark_pages(16 * PAGE_SIZE, 1, 1);
    expect_bitmap(0x5a, 0x5a);
    mark_pages(15 * PAGE_SIZE, 2, 0);
    expect_bitmap(0x5a, 0x5a);
    mark_pages(24 * PAGE_SIZE, 1, 1);
    expect_bitmap(0x5a, 0x5a);
    mark_pages(PAGE_SIZE, UINT64_MAX, 0);
    expect_bitmap(0x5a, 0x5a);
    mark_pages(UINT64_MAX - (PAGE_SIZE - 1), 1, 0);
    expect_bitmap(0x5a, 0x5a);
    assert(used != 0);

    physical_pages_map->mapsize = 0;
    mark_pages(0, 1, 1);
    assert(physical_pages_map->bitmap[0] == 0x5a);
    physical_pages_map->mapsize = 2;
}

static void test_print(void)
{
    setup_map(33, 0);
    for (size_t i = 0; i < 33; i++)
        physical_pages_map->bitmap[i] = (unsigned char)i;
    print_physical_pages_map();
    const char *data = strchr(output, '\n');
    assert(data != NULL);
    assert(strcmp(data + 1,
                  "00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f "
                  "10 11 12 13 14 15 16 17 18 19 1a 1b 1c 1d 1e 1f \n"
                  "20 \n") == 0);
    for (size_t i = 0; i < 33; i++)
        assert(physical_pages_map->bitmap[i] == i);
    assert(physical_pages_map->bitmap[33] == 0xa5);

    used = 0;
    output[0] = '\0';
    physical_pages_map->mapsize = 0;
    print_physical_pages_map();
    data = strchr(output, '\n');
    assert(data != NULL && data[1] == '\0');
}

int main(int argc, char **argv)
{
    assert(argc == 2);
    if (strcmp(argv[1], "uninitialized") == 0)
        test_uninitialized();
    else if (strcmp(argv[1], "mark") == 0)
        test_mark();
    else if (strcmp(argv[1], "clear") == 0)
        test_clear();
    else if (strcmp(argv[1], "zero_count") == 0)
        test_zero_count();
    else if (strcmp(argv[1], "bounds") == 0)
        test_bounds();
    else if (strcmp(argv[1], "print") == 0)
        test_print();
    else
        return 1;

    if (physical_pages_map != (physical_pages_map_t *)-1)
        free(physical_pages_map);
    return 0;
}
