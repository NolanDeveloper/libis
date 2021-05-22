#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

#include "libis_internal.h"

struct Libis_ {
    int dummy;
};

// Bytes get read lazily from source into the buffer usually one by one.
// The buffer is only required for lookahead. When the user needs to
// look ahead by n bytes, n bytes get read into the buffer. Subsequent
// libis_read_* calls first consume bytes from the buffer.
struct LibisInputStream_ {
    LibisSource *source;
    char *buffer;
    size_t buffer_size; // Number of bytes filled into buffer
    size_t buffer_capacity; // Buffer length
    unsigned bit_offset; // Bit offset from start of buffer (always less than CHAR_BIT)
};

LibisError libis_handle_internal_error(LibisError err) {
    switch (err) {
    case LIBIS_ERROR_OK:
        return LIBIS_ERROR_OK;
    case LIBIS_ERROR_OUT_OF_MEMORY:
        return LIBIS_ERROR_OUT_OF_MEMORY;
    case LIBIS_ERROR_BAD_ARGUMENT:
        abort();
    case LIBIS_ERROR_IO:
        return LIBIS_ERROR_IO;
    case LIBIS_ERROR_TOO_FAR:
        return LIBIS_ERROR_TOO_FAR;
    case LIBIS_ERROR_HANGING_BITS:
        return LIBIS_ERROR_HANGING_BITS;
    }
    abort();
}

LibisError libis_start(Libis **libis) {
    LibisError err = LIBIS_ERROR_OK;
    Libis *result = NULL;
    if (!libis) {
        err = LIBIS_ERROR_BAD_ARGUMENT;
        goto end;
    }
    result = malloc(sizeof(Libis));
    if (!result) {
        err = LIBIS_ERROR_OUT_OF_MEMORY;
        goto end;
    }
    *libis = result;
    result = NULL;
end:
    E(libis_finish(&result));
    return err;
}

LibisError libis_finish(Libis **libis) {
    LibisError err = LIBIS_ERROR_OK;
    if (!libis) {
        err = LIBIS_ERROR_BAD_ARGUMENT;
        goto end;
    }
    if (*libis) {
        free(*libis);
        *libis = NULL;
    }
end:
    return err;
}

LibisError libis_source_destroy(Libis *libis, LibisSource **source) {
    LibisError err = LIBIS_ERROR_OK;
    if (!libis || !source) {
        err = LIBIS_ERROR_BAD_ARGUMENT;
        goto end;
    }
    if (*source) {
        (*source)->free(libis, *source);
        *source = NULL;
    }
end:
    return err;
}

LibisError libis_create(Libis *libis, LibisInputStream **input, LibisSource **source, size_t lookahead) {
    LibisError err = LIBIS_ERROR_OK;
    LibisInputStream *result = NULL;
    char *buffer = NULL;
    if (!libis || !input || !source) {
        err = LIBIS_ERROR_BAD_ARGUMENT;
        goto end;
    }
    if (lookahead < 2) {
        lookahead = 2;
    }
    buffer = malloc(lookahead);
    if (!buffer) {
        err = LIBIS_ERROR_OUT_OF_MEMORY;
        goto end;
    }
    result = malloc(sizeof(LibisInputStream));
    if (!result) {
        err = LIBIS_ERROR_OUT_OF_MEMORY;
        goto end;
    }
    result->source = *source;
    result->buffer = buffer;
    result->buffer_size = 0;
    result->buffer_capacity = lookahead;
    result->bit_offset = 0;
    *input = result;
    *source = NULL;
    buffer = NULL;
    result = NULL;
end:
    free(buffer);
    free(result);
    return err;
}

LibisError libis_destroy(Libis *libis, LibisInputStream **input) {
    LibisError err = LIBIS_ERROR_OK;
    if (!libis || !input) {
        err = LIBIS_ERROR_BAD_ARGUMENT;
        goto end;
    }
    if (!*input) {
        goto end;
    }
    E((*input)->source->free(libis, (*input)->source));
    free((*input)->buffer);
    free(*input);
end:
    return err;
}

// Fill buffer with at least length bytes from source.
static LibisError libis_prepare_block(Libis *libis, LibisInputStream *input, bool *eof, size_t size) {
    LibisError err = LIBIS_ERROR_OK;
    if (!libis || !input) {
        err = LIBIS_ERROR_BAD_ARGUMENT;
        goto end;
    }
    *eof = false;
    if (input->buffer_capacity < size) {
        err = LIBIS_ERROR_TOO_FAR;
        goto end;
    }
    while (input->buffer_size < size) {
        err = input->source->read(libis, input->source, eof, &input->buffer[input->buffer_size]);
        if (*eof || err) {
            goto end;
        }
        ++input->buffer_size;
    }
end:
    return err;
}

LibisError libis_lookahead(Libis *libis, LibisInputStream *input, bool *eof, size_t offset, char *out) {
    LibisError err = LIBIS_ERROR_OK;
    if (!libis || !input || !out) {
        err = LIBIS_ERROR_BAD_ARGUMENT;
        goto end;
    }
    *out = 0;
    err = E(libis_prepare_block(libis, input, eof, offset));
    if (*eof || err) {
        goto end;
    }
    *out = input->buffer[offset - 1];
end:
    return err;
}

LibisError libis_skip_char(Libis *libis, LibisInputStream *input, bool *eof, char *out) {
    LibisError err = LIBIS_ERROR_OK;
    if (!libis || !input || !out) {
        err = LIBIS_ERROR_BAD_ARGUMENT;
        goto end;
    }
    err = E(libis_read_char(libis, input, eof, out));
    if (err) goto end;
    err = E(libis_lookahead(libis, input, eof, 1, out));
    if (err) goto end;
end:
    return err;
}

LibisError libis_read_char(Libis *libis, LibisInputStream *input, bool *eof, char *out) {
    LibisError err = LIBIS_ERROR_OK;
    if (!libis || !input || !out) {
        err = LIBIS_ERROR_BAD_ARGUMENT;
        goto end;
    }
    *out = 0;
    if (input->bit_offset != 0) {
        err = LIBIS_ERROR_HANGING_BITS;
        goto end;
    }
    err = E(libis_prepare_block(libis, input, eof, 1));
    if (*eof || err) {
        goto end;
    }
    *out = input->buffer[0];
    assert(input->buffer_size > 0);
    memmove(input->buffer, input->buffer + 1, input->buffer_size - 1);
    --input->buffer_size;
end:
    return err;
}

LibisError libis_read_bits(Libis *libis, LibisInputStream *input, bool *eof, unsigned nbits, unsigned *out) {
//        current byte             next byte                      current byte             next byte
// |--+--+--+--+--+--+--+--|--+--+--+--+--+--+--+--|       |--+--+--+--+--+--+--+--|--+--+--+--+--+--+--+--|
// |-----------|-----------|--------|                      |-----------|--------|
//  bit_offset | first      second  |                       bit_offset | first  |
//             | portion    portion |                                  | portion|
//             |--------------------|                                  |--------|
//                     nbits                                             nbits
    LibisError err = LIBIS_ERROR_OK;
    unsigned char c1, c2;
    if (!libis || !input || !eof || !out || CHAR_BIT <= nbits) {
        err = LIBIS_ERROR_BAD_ARGUMENT;
        goto end;
    }
    *out = 0;
    err = E(libis_lookahead(libis, input, eof, 1, (char *) &c1));
    if (*eof || err) {
        goto end;
    }
    unsigned bits_hanging = CHAR_BIT - input->bit_offset;
    unsigned first_portion = bits_hanging < nbits ? bits_hanging : nbits;
    unsigned second_portion = nbits - first_portion;
    c1 &= (1u << bits_hanging) - 1;
    c1 >>= CHAR_BIT - input->bit_offset - first_portion;
    if (!second_portion) {
        c2 = 0;
    } else {
        err = E(libis_lookahead(libis, input, eof, 2, (char *) &c2));
        if (*eof || err) {
            goto end;
        }
        c2 >>= CHAR_BIT - second_portion;
    }
    *out = c1 << second_portion | c2;
    input->bit_offset += nbits;
    if (CHAR_BIT <= input->bit_offset) {
        unsigned new_bit_offset = input->bit_offset - CHAR_BIT;
        input->bit_offset = 0;
        err = E(libis_read_char(libis, input, eof, (char *) &c1));
        assert(!*eof);
        input->bit_offset = new_bit_offset;
        if (err) goto end;
    }
end:
    return err;
}

LibisError libis_read_u8(Libis *libis, LibisInputStream *input, bool *eof, uint8_t *out) {
    LibisError err = LIBIS_ERROR_OK;
    unsigned char c;
    if (!libis || !input || !out) {
        err = LIBIS_ERROR_BAD_ARGUMENT;
        goto end;
    }
    *out = 0;
    err = E(libis_read_char(libis, input, eof, (char *) &c));
    if (*eof || err) {
        goto end;
    }
    *out = c;
end:
    return err;
}

LibisError libis_read_u16_le(Libis *libis, LibisInputStream *input, bool *eof, uint16_t *out) {
    LibisError err = LIBIS_ERROR_OK;
    uint8_t c1, c2;
    if (!libis || !input || !out) {
        err = LIBIS_ERROR_BAD_ARGUMENT;
        goto end;
    }
    *out = 0;
    err = E(libis_read_u8(libis, input, eof, &c1));
    if (*eof || err) {
        goto end;
    }
    err = E(libis_read_u8(libis, input, eof, &c2));
    if (*eof || err) {
        goto end;
    }
    *out = ((uint16_t) c2 << CHAR_BIT) | (uint16_t) c1;
end:
    return err;
}

LibisError libis_read_u16_be(Libis *libis, LibisInputStream *input, bool *eof, uint16_t *out) {
    LibisError err = LIBIS_ERROR_OK;
    uint8_t c1, c2;
    if (!libis || !input || !out) {
        err = LIBIS_ERROR_BAD_ARGUMENT;
        goto end;
    }
    *out = 0;
    err = E(libis_read_u8(libis, input, eof, &c1));
    if (*eof || err) {
        goto end;
    }
    err = E(libis_read_u8(libis, input, eof, &c2));
    if (*eof || err) {
        goto end;
    }
    *out = ((uint16_t) c1 << CHAR_BIT) | (uint16_t) c2;
end:
    return err;
}

LibisError libis_read_u32_le(Libis *libis, LibisInputStream *input, bool *eof, uint32_t *out) {
    LibisError err = LIBIS_ERROR_OK;
    uint16_t c1, c2;
    if (!libis || !input || !out) {
        err = LIBIS_ERROR_BAD_ARGUMENT;
        goto end;
    }
    *out = 0;
    err = E(libis_read_u16_le(libis, input, eof, &c1));
    if (*eof || err) {
        goto end;
    }
    err = E(libis_read_u16_le(libis, input, eof, &c2));
    if (*eof || err) {
        goto end;
    }
    *out = ((uint32_t) c2 << (sizeof(uint16_t) * CHAR_BIT)) | (uint32_t) c1;
end:
    return err;
}

LibisError libis_read_u32_be(Libis *libis, LibisInputStream *input, bool *eof, uint32_t *out) {
    LibisError err = LIBIS_ERROR_OK;
    uint16_t c1, c2;
    if (!libis || !input || !out) {
        err = LIBIS_ERROR_BAD_ARGUMENT;
        goto end;
    }
    *out = 0;
    err = E(libis_read_u16_be(libis, input, eof, &c1));
    if (*eof || err) {
        goto end;
    }
    err = E(libis_read_u16_be(libis, input, eof, &c2));
    if (*eof || err) {
        goto end;
    }
    *out = ((uint32_t) c1 << (sizeof(uint16_t) * CHAR_BIT)) | (uint32_t) c2;
end:
    return err;
}

LibisError libis_read_u64_le(Libis *libis, LibisInputStream *input, bool *eof, uint64_t *out) {
    LibisError err = LIBIS_ERROR_OK;
    uint32_t c1, c2;
    if (!libis || !input || !out) {
        err = LIBIS_ERROR_BAD_ARGUMENT;
        goto end;
    }
    *out = 0;
    err = E(libis_read_u32_le(libis, input, eof, &c1));
    if (*eof || err) {
        goto end;
    }
    err = E(libis_read_u32_le(libis, input, eof, &c2));
    if (*eof || err) {
        goto end;
    }
    *out = ((uint64_t) c2 << (sizeof(uint32_t) * CHAR_BIT)) | (uint64_t) c1;
end:
    return err;
}

LibisError libis_read_u64_be(Libis *libis, LibisInputStream *input, bool *eof, uint64_t *out) {
    LibisError err = LIBIS_ERROR_OK;
    uint32_t c1, c2;
    if (!libis || !input || !out) {
        err = LIBIS_ERROR_BAD_ARGUMENT;
        goto end;
    }
    *out = 0;
    err = E(libis_read_u32_be(libis, input, eof, &c1));
    if (*eof || err) {
        goto end;
    }
    err = E(libis_read_u32_be(libis, input, eof, &c2));
    if (*eof || err) {
        goto end;
    }
    *out = ((uint64_t) c1 << (sizeof(uint32_t) * CHAR_BIT)) | (uint64_t) c2;
end:
    return err;
}
