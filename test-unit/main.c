#include <assert.h>
#include <fcntl.h>
#include <libis.h>
#include <stdio.h>

static LibisError err;

static Libis *libis;

// 0b11011100 == 0xDC
// 1 10 11 100110 11100
// 1 2  3  38     28
static const char buffer[] =
        "\xDC\xDC"
        "\x10"
        "\x20\x21"
        "\x31\x30"
        "\x40\x41\x42\x43"
        "\x53\x52\x51\x50"
        "\x60\x61\x62\x63\x64\x65\x66\x67"
        "\x77\x76\x75\x74\x73\x72\x71\x70"
        "X";

static void test(LibisSource **source) {
    LibisInputStream *input;

    bool eof;
    unsigned bits;
    char c;
    uint8_t u8;
    uint16_t u16;
    uint32_t u32;
    uint64_t u64;

    err = libis_create(libis, &input, source, 1);
    assert(LIBIS_ERROR_OK == err);

    err = libis_read_bits(libis, input, &eof, 1, &bits);
    assert(!eof && LIBIS_ERROR_OK == err);
    assert(bits == 1);

    err = libis_read_bits(libis, input, &eof, 2, &bits);
    assert(!eof && LIBIS_ERROR_OK == err);
    assert(bits == 2);

    err = libis_read_bits(libis, input, &eof, 2, &bits);
    assert(!eof && LIBIS_ERROR_OK == err);
    assert(bits == 3);

    err = libis_read_bits(libis, input, &eof, 6, &bits);
    assert(!eof && LIBIS_ERROR_OK == err);
    assert(bits == 38);

    err = libis_read_bits(libis, input, &eof, 5, &bits);
    assert(!eof && LIBIS_ERROR_OK == err);
    assert(bits == 28);

    err = libis_read_u8(libis, input, &eof, &u8);
    assert(!eof && LIBIS_ERROR_OK == err);
    assert(u8 == 0x10);

    err = libis_read_u16_le(libis, input, &eof, &u16);
    assert(!eof && LIBIS_ERROR_OK == err);
    assert(u16 == 0x2120);

    err = libis_read_u16_be(libis, input, &eof, &u16);
    assert(!eof && LIBIS_ERROR_OK == err);
    assert(u16 == 0x3130);

    err = libis_read_u32_le(libis, input, &eof, &u32);
    assert(!eof && LIBIS_ERROR_OK == err);
    assert(u32 == 0x43424140);

    err = libis_read_u32_be(libis, input, &eof, &u32);
    assert(!eof && LIBIS_ERROR_OK == err);
    assert(u32 == 0x53525150);

    err = libis_read_u64_le(libis, input, &eof, &u64);
    assert(!eof && LIBIS_ERROR_OK == err);
    assert(u64 == 0x6766656463626160);

    err = libis_read_u64_be(libis, input, &eof, &u64);
    assert(!eof && LIBIS_ERROR_OK == err);
    assert(u64 == 0x7776757473727170);

    err = libis_read_char(libis, input, &eof, &c);
    assert(!eof && LIBIS_ERROR_OK == err);
    assert(c == 'X');

    err = libis_read_char(libis, input, &eof, &c);
    assert(eof && LIBIS_ERROR_OK == err);
    assert(c == '\0');

    err = libis_destroy(libis, &input);
    assert(LIBIS_ERROR_OK == err);
}

int main() {
    err = libis_start(&libis);
    assert(LIBIS_ERROR_OK == err);

    LibisSource *source;

    err = libis_source_create_from_buffer(libis, &source, buffer, sizeof(buffer) - 1, false);
    assert(LIBIS_ERROR_OK == err);
    test(&source);

    FILE *file = fopen("test.bin", "w+b");
    assert(file);
    size_t items = fwrite(buffer, sizeof(buffer) - 1, 1, file);
    assert(1 == items);
    assert(!fseek(file, 0, SEEK_SET));
    err = libis_source_create_from_file(libis, &source, &file);
    assert(LIBIS_ERROR_OK == err);
    test(&source);

#if defined(__linux__)
    int fd = open("test.bin", O_RDONLY);
    assert(0 <= fd);
    err = libis_source_create_from_file_descriptor(libis, &source, &fd);
    assert(LIBIS_ERROR_OK == err);
    test(&source);
#endif

    err = libis_finish(&libis);
    assert(LIBIS_ERROR_OK == err);
    return 0;
}