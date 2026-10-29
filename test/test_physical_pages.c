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
    physical_pages_map->total_pages = bytes * 4;
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
    mark_pages(0, 1, PAGE_USED);
    expect_bitmap(0x01, 0x00);
    mark_pages(2 * PAGE_SIZE, 4, PAGE_USED);
    expect_bitmap(0x51, 0x05);
    mark_pages(2 * PAGE_SIZE, 4, PAGE_USED);
    expect_bitmap(0x51, 0x05);
    mark_pages(7 * PAGE_SIZE, 1, PAGE_RESERVED);
    expect_bitmap(0x51, 0x85);
    mark_pages(0, 8, PAGE_USED);
    expect_bitmap(0x55, 0x55);
}

static void test_clear(void)
{
    setup_map(2, 0xff);
    mark_pages(2 * PAGE_SIZE, 4, PAGE_FREE);
    expect_bitmap(0x0f, 0xf0);
    mark_pages(2 * PAGE_SIZE, 4, PAGE_FREE);
    expect_bitmap(0x0f, 0xf0);
    mark_pages(7 * PAGE_SIZE, 1, PAGE_FREE);
    expect_bitmap(0x0f, 0x30);
    mark_pages(0, 8, PAGE_FREE);
    expect_bitmap(0, 0);
}

static void test_zero_count(void)
{
    setup_map(2, 0x5a);
    mark_pages(0, 0, PAGE_USED);
    mark_pages(8 * PAGE_SIZE, 0, PAGE_FREE);
    expect_bitmap(0x5a, 0x5a);
}

static void test_allocate(void)
{
    assert(allocate_page(1) == NULL);

    setup_map(2, 0);
    physical_pages_map->total_pages = 8;

    assert(allocate_page(0) == NULL);
    assert(physical_pages_map->bitmap[0] == 0x00);

    assert((uintptr_t)allocate_page(1) == PAGE_SIZE);
    assert(physical_pages_map->bitmap[0] == 0x04);

    assert((uintptr_t)allocate_page(2) == 2 * PAGE_SIZE);
    assert(physical_pages_map->bitmap[0] == 0x54);

    assert((uintptr_t)allocate_page(4) == 4 * PAGE_SIZE);
    assert(physical_pages_map->bitmap[0] == 0x54);
    assert(physical_pages_map->bitmap[1] == 0x55);
    assert(allocate_page(2) == NULL);
    assert(allocate_page(1) == NULL);
}

static void test_allocate_fragmented(void)
{
    setup_map(2, 0xff);
    physical_pages_map->total_pages = 8;
    mark_pages(PAGE_SIZE, 1, PAGE_FREE);
    mark_pages(3 * PAGE_SIZE, 1, PAGE_FREE);
    mark_pages(5 * PAGE_SIZE, 1, PAGE_FREE);

    assert(allocate_page(2) == NULL);
    assert(physical_pages_map->bitmap[0] == 0x33);
    assert(physical_pages_map->bitmap[1] == 0xf3);
}

static void test_allocate_bounds(void)
{
    setup_map(2, 0xff);
    physical_pages_map->total_pages = 8;
    mark_pages(6 * PAGE_SIZE, 2, PAGE_FREE);

    assert((uintptr_t)allocate_page(2) == 6 * PAGE_SIZE);
    assert(physical_pages_map->bitmap[0] == 0xff);
    assert(physical_pages_map->bitmap[1] == 0x5f);

    mark_pages(2 * PAGE_SIZE, 1, PAGE_FREE);
    assert(allocate_page(UINT64_MAX) == NULL);
    assert(physical_pages_map->bitmap[0] == 0xcf);
}

static void test_allocate_invalid_map(void)
{
    setup_map(1, 0);
    physical_pages_map->total_pages = 9;
    assert(allocate_page(1) == NULL);
    assert(allocate_page_at(PAGE_SIZE, 1) == NULL);
    assert(physical_pages_map->bitmap[0] == 0);
}

static void test_allocate_at(void)
{
    assert(allocate_page_at(PAGE_SIZE, 1) == NULL);

    setup_map(4, 0xff);
    physical_pages_map->total_pages = 16;
    mark_pages(PAGE_SIZE, 15, PAGE_FREE);

    assert(allocate_page_at(PAGE_SIZE, 0) == NULL);
    assert(allocate_page_at(0, 1) == NULL);
    assert(allocate_page_at(PAGE_SIZE + 1, 1) == NULL);
    assert(physical_pages_map->bitmap[0] == 0x03);
    assert((uintptr_t)allocate_page_at(PAGE_SIZE, 2) == PAGE_SIZE);
    assert(physical_pages_map->bitmap[0] == 0x17);
    assert(physical_pages_map->bitmap[1] == 0x00);

    assert(allocate_page_at(PAGE_SIZE, 1) == NULL);
    assert((uintptr_t)allocate_page_at(14 * PAGE_SIZE, 2) == 14 * PAGE_SIZE);
    assert(physical_pages_map->bitmap[3] == 0x50);

    assert(allocate_page_at(15 * PAGE_SIZE, 2) == NULL);
    assert(allocate_page_at(UINT64_MAX - (PAGE_SIZE - 1), 2) == NULL);
    assert(physical_pages_map->bitmap[3] == 0x50);
}

static void test_free_page(void)
{
    setup_map(2, 0xff);
    physical_pages_map->total_pages = 8;
    mark_pages(PAGE_SIZE, 3, PAGE_FREE);

    void *first = allocate_page(2);
    assert((uintptr_t)first == PAGE_SIZE);
    assert(physical_pages_map->bitmap[0] == 0x17);

    free_page(first, 2);
    assert(physical_pages_map->bitmap[0] == 0x03);
    assert(physical_pages_map->bitmap[1] == 0xff);
    assert(physical_pages_map->bitmap[2] == 0xa5);

    assert((uintptr_t)allocate_page(2) == PAGE_SIZE);
    assert(physical_pages_map->bitmap[0] == 0x17);
}

static void test_free_page_rejects_invalid_range(void)
{
    setup_map(2, 0xaa);
    physical_pages_map->total_pages = 8;
    mark_pages(PAGE_SIZE, 1, PAGE_USED);
    unsigned char before[2] = {
        physical_pages_map->bitmap[0],
        physical_pages_map->bitmap[1],
    };

    free_page((void *)(PAGE_SIZE + 1), 1);
    free_page((void *)PAGE_SIZE, 2);
    free_page((void *)(7 * PAGE_SIZE), 2);
    free_page((void *)(UINT64_MAX & ~(PAGE_SIZE - 1)), 1);
    free_page((void *)PAGE_SIZE, 0);

    assert(physical_pages_map->bitmap[0] == before[0]);
    assert(physical_pages_map->bitmap[1] == before[1]);
    assert(physical_pages_map->bitmap[2] == 0xa5);

    free_page((void *)PAGE_SIZE, 1);
    assert(physical_pages_map->bitmap[0] == 0xa2);
    free_page((void *)PAGE_SIZE, 1);
    assert(physical_pages_map->bitmap[0] == 0xa2);
}

static void test_bounds(void)
{
    setup_map(2, 0x5a);
    mark_pages(8 * PAGE_SIZE, 1, PAGE_USED);
    expect_bitmap(0x5a, 0x5a);
    mark_pages(7 * PAGE_SIZE, 2, PAGE_FREE);
    expect_bitmap(0x5a, 0x5a);
    mark_pages(12 * PAGE_SIZE, 1, PAGE_USED);
    expect_bitmap(0x5a, 0x5a);
    mark_pages(PAGE_SIZE + 1, 1, PAGE_USED);
    mark_pages(PAGE_SIZE, 1, (page_status)-1);
    expect_bitmap(0x5a, 0x5a);
    mark_pages(PAGE_SIZE, UINT64_MAX, PAGE_FREE);
    expect_bitmap(0x5a, 0x5a);
    mark_pages(UINT64_MAX - (PAGE_SIZE - 1), 1, PAGE_FREE);
    expect_bitmap(0x5a, 0x5a);
    assert(used != 0);

    physical_pages_map->mapsize = 0;
    mark_pages(0, 1, PAGE_USED);
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
    else if (strcmp(argv[1], "allocate") == 0)
        test_allocate();
    else if (strcmp(argv[1], "allocate_fragmented") == 0)
        test_allocate_fragmented();
    else if (strcmp(argv[1], "allocate_bounds") == 0)
        test_allocate_bounds();
    else if (strcmp(argv[1], "allocate_at") == 0)
        test_allocate_at();
    else if (strcmp(argv[1], "free_page") == 0)
        test_free_page();
    else if (strcmp(argv[1], "free_page_invalid") == 0)
        test_free_page_rejects_invalid_range();
    else if (strcmp(argv[1], "allocate_invalid_map") == 0)
        test_allocate_invalid_map();
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
