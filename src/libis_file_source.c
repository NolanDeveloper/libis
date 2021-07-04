#include <stdlib.h>
#include <libis.h>

#include "libis_internal.h"

// LibisSource for a FILE
typedef struct {
    LibisSource source;
    FILE *file;
} LibisFileSource;

// see LibSource::read
static LibisError libis_file_source_read(Libis *libis, LibisSource *source, bool *eof, char *c) {
    LibisError err = LIBIS_ERROR_OK;
    LibisFileSource *file_source = (LibisFileSource *) source;
    if (!libis || !source || !c) {
        err = LIBIS_ERROR_BAD_ARGUMENT;
        goto end;
    }
    size_t nbytes = fread(c, 1, 1, file_source->file);
    if (nbytes != 1) {
        if (feof(file_source->file)) {
            *eof = true;
            *c = '\0';
        } else {
            err = LIBIS_ERROR_IO;
        }
        goto end;
    }
    *eof = false;
end:
    return err;
}

// see LibSource::free
static LibisError libis_file_source_free(Libis *libis, LibisSource *source) {
    LibisError err = LIBIS_ERROR_OK;
    if (!libis || !source) {
        goto end;
    }
    LibisFileSource *file_source = (LibisFileSource *) source;
    if (file_source->file) {
        fclose(file_source->file);
        file_source->file = NULL;
    }
    free(source);
end:
    return err;
}

LibisError libis_source_create_from_file(Libis *libis, LibisSource **source, FILE **file) {
    LibisError err = LIBIS_ERROR_OK;
    LibisFileSource *result = NULL;
    if (!libis || !file || !source) {
        err = LIBIS_ERROR_BAD_ARGUMENT;
        goto end;
    }
    result = malloc(sizeof(LibisFileSource));
    if (!result) {
        err = LIBIS_ERROR_OUT_OF_MEMORY;
        goto end;
    }
    result->source.read = libis_file_source_read;
    result->source.free = libis_file_source_free;
    result->file = *file;
    *source = (LibisSource *) result;
    *file = NULL;
    result = NULL;
end:
    if (file && *file) {
        fclose(*file);
    }
    free(result);
    return err;
}

