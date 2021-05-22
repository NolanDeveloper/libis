/* LIBrary of Input Streams both text and binary with lookahead support.
 *
 * The library provides abstraction over different input ways which include:
 * + reading a memory buffer from left to right
 * + reading from a FILE * (not seekable too)
 * + reading from a file descriptor (on Linux)
 *
 * Other ways like reading from a HANDLE on Windows may be added easily.
 */

#ifndef LIBIS_H
#define LIBIS_H

#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>

#define LIBIS_LOOKAHEAD_MIN 2

// Structure that must be passed to all library functions.
typedef struct Libis_ Libis;

// Interface of input source: FILE, buffer, file descriptor, HANDLE...
// Something that we can read from byte by byte.
typedef struct LibisSource_ LibisSource;

// Interface for reading bytes and bits from source with lookahead capability.
typedef struct LibisInputStream_ LibisInputStream;

typedef enum {
    LIBIS_ERROR_OK,
    LIBIS_ERROR_OUT_OF_MEMORY,
    LIBIS_ERROR_BAD_ARGUMENT,
    LIBIS_ERROR_IO, // underlying system IO error
    LIBIS_ERROR_TOO_FAR, // attempt to look ahead too far
    LIBIS_ERROR_HANGING_BITS, // a group of calls to libis_read_bits() didn't end up reading whole number of bytes
} LibisError;

// Initialize *libis.
LibisError libis_start(Libis **libis);

// Free resources taken by *libis.
LibisError libis_finish(Libis **libis);

// Create LibisSource from a file.
LibisError libis_source_create_from_file(Libis *libis, LibisSource **source, FILE **file);

// Create LibisSource from a buffer.
// own - should we free buffer when the source gets freed.
LibisError libis_source_create_from_buffer(
        Libis *libis, LibisSource **source, const char *buffer, size_t size, bool own);

#if defined(__linux__)
// Create LibisSource from a file descriptor.
LibisError libis_source_create_from_file_descriptor(Libis *libis, LibisSource **source, int *file_descriptor);
#endif

// Free resources taken by LibisSource.
LibisError libis_source_destroy(Libis *libis, LibisSource **source);

// Create LibisInputStream from LibisSource capable to look ahead by lookahead bytes.
LibisError libis_create(Libis *libis, LibisInputStream **input, LibisSource **source, size_t lookahead);

// Free resources taken by LibisInputStream.
LibisError libis_destroy(Libis *libis, LibisInputStream **input);

// Look ahead by offset bytes. offset == 1 is the next byte to read.
// *eof sets to whether end of file is reached. If so *out sets to '\0'.
LibisError libis_lookahead(Libis *libis, LibisInputStream *input, bool *eof, size_t offset, char *out);

// Read next character from LibisInputStream.
// *eof sets to whether end of file is reached. If so *out sets to '\0'.
LibisError libis_read_char(Libis *libis, LibisInputStream *input, bool *eof, char *out);

// Read one character from input stream and lookahead the next one.
// *eof sets to whether end of file is reached. If so *out sets to '\0'.
LibisError libis_skip_char(Libis *libis, LibisInputStream *input, bool *eof, char *out);

// Read a portion of bits (less than CHAR_BIT) from input stream.
// Calls to this function must be grouped such that whole number of bytes gets read in total.
// *eof sets to whether end of file is reached. If so *out sets to '\0'.
LibisError libis_read_bits(Libis *libis, LibisInputStream *input, bool *eof, unsigned nbits, unsigned *out);

// Read next byte from input stream.
// *eof sets to whether end of file is reached. If so *out sets to '\0'.
LibisError libis_read_u8(Libis *libis, LibisInputStream *input, bool *eof, uint8_t *out);

// Read next 2 bytes in little endian from input stream.
// *eof sets to whether end of file is reached. If so *out sets to '\0'.
LibisError libis_read_u16_le(Libis *libis, LibisInputStream *input, bool *eof, uint16_t *out);

// Read next 2 bytes in big endian from input stream.
// *eof sets to whether end of file is reached. If so *out sets to '\0'.
LibisError libis_read_u16_be(Libis *libis, LibisInputStream *input, bool *eof, uint16_t *out);

// Read next 4 bytes in little endian from input stream.
// *eof sets to whether end of file is reached. If so *out sets to '\0'.
LibisError libis_read_u32_le(Libis *libis, LibisInputStream *input, bool *eof, uint32_t *out);

// Read next 4 bytes in big endian from input stream.
// *eof sets to whether end of file is reached. If so *out sets to '\0'.
LibisError libis_read_u32_be(Libis *libis, LibisInputStream *input, bool *eof, uint32_t *out);

// Read next 8 bytes in little endian from input stream.
// *eof sets to whether end of file is reached. If so *out sets to '\0'.
LibisError libis_read_u64_le(Libis *libis, LibisInputStream *input, bool *eof, uint64_t *out);

// Read next 8 bytes in big endian from input stream.
// *eof sets to whether end of file is reached. If so *out sets to '\0'.
LibisError libis_read_u64_be(Libis *libis, LibisInputStream *input, bool *eof, uint64_t *out);

#endif
