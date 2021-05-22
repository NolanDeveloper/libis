#include <stdlib.h>
#include <unistd.h>
#include "libis_source.h"

// LibisSource for a file descriptor
typedef struct {
    LibisSource source;
    int file_descriptor;
} LibisFileDescriptorSource;

// see LibSource::read
LibisError libis_file_descriptor_source_read(Libis *libis, LibisSource *source, bool *eof, char *c) {
    LibisError err = LIBIS_ERROR_OK;
    LibisFileDescriptorSource *file_descriptor_source = (LibisFileDescriptorSource *) source;
    if (!libis || !source || !eof || !c) {
        err = LIBIS_ERROR_BAD_ARGUMENT;
        goto end;
    }
    *eof = false;
    *c = '\0';
    int n = read(file_descriptor_source->file_descriptor, c, 1);
    if (n < 0) {
        err = LIBIS_ERROR_IO;
        goto end;
    }
    if (!n) {
        *eof = true;
        *c = '\0';
    }
end:
    return err;
}

// see LibSource::free
LibisError libis_file_descriptor_source_free(Libis *libis, LibisSource *source) {
    LibisError err = LIBIS_ERROR_OK;
    if (!libis || !source) {
        err = LIBIS_ERROR_BAD_ARGUMENT;
        goto end;
    }
    LibisFileDescriptorSource *file_descriptor_source = (LibisFileDescriptorSource *) source;
    if (file_descriptor_source) {
        close(file_descriptor_source->file_descriptor);
    }
    free(source);
end:
    return err;
}

LibisError libis_source_create_from_file_descriptor(Libis *libis, LibisSource **source, int *file_descriptor) {
    LibisError err = LIBIS_ERROR_OK;
    LibisFileDescriptorSource *result = NULL;
    if (!libis || !source) {
        err = LIBIS_ERROR_BAD_ARGUMENT;
        goto end;
    }
    result = malloc(sizeof(LibisFileDescriptorSource));
    if (!result) {
        err = LIBIS_ERROR_OUT_OF_MEMORY;
        goto end;
    }
    result->source.read = libis_file_descriptor_source_read;
    result->source.free = libis_file_descriptor_source_free;
    result->file_descriptor = *file_descriptor;
    *source = (LibisSource *) result;
    *file_descriptor = -1;
    result = NULL;
end:
    close(*file_descriptor);
    free(result);
    return err;
}