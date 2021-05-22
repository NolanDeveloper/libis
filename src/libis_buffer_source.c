#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <libis.h>

#include "libis_internal.h"

// LibisSource for a buffer
typedef struct {
    LibisSource source;
    const char *buffer;
    size_t size; // length of buffer
    size_t offset; // read position inside buffer
    bool own; // should we free buffer when this source gets freed
} LibisBufferSource;

// see LibisSource::read
static LibisError libis_buffer_source_read(Libis *libis, LibisSource *source, bool *eof, char *c) {
    LibisError err = LIBIS_ERROR_OK;
    LibisBufferSource *buffer_source = (LibisBufferSource *) source;
    if (!libis || !source) {
        err = LIBIS_ERROR_BAD_ARGUMENT;
        goto end;
    }
    assert(buffer_source->offset <= buffer_source->size);
    if (buffer_source->offset == buffer_source->size) {
        *eof = true;
        *c = '\0';
        goto end;
    }
    *c = buffer_source->buffer[buffer_source->offset];
    ++buffer_source->offset;
    *eof = false;
end:
    return err;
}

// see LibisSource::free
static LibisError libis_buffer_source_free(Libis *libis, LibisSource *source) {
    LibisError err = LIBIS_ERROR_OK;
    if (!libis || !source) {
        goto end;
    }
    LibisBufferSource *buffer_source = (LibisBufferSource *) source;
    if (buffer_source->own) {
        free((void *) buffer_source->buffer);
    }
    free(source);
end:
    return err;
}

LibisError libis_source_create_from_buffer(
        Libis *libis, LibisSource **source, const char *buffer, size_t size, bool own) {
    LibisError err = LIBIS_ERROR_OK;
    LibisBufferSource *buffer_source = NULL;
    if (!libis || !source || !buffer) {
        err = LIBIS_ERROR_BAD_ARGUMENT;
        goto end;
    }
    buffer_source = malloc(sizeof(LibisBufferSource));
    if (!buffer_source) {
        err = LIBIS_ERROR_OUT_OF_MEMORY;
        goto end;
    }
    buffer_source->source.read = libis_buffer_source_read;
    buffer_source->source.free = libis_buffer_source_free;
    buffer_source->buffer = buffer;
    buffer_source->size = size;
    buffer_source->offset = 0;
    buffer_source->own = own;
    *source = (LibisSource *) buffer_source;
    buffer_source = NULL;
end:
    libis_buffer_source_free(libis, (LibisSource *) buffer_source);
    return err;
}
