#ifndef LIBIS_SOURCE_H
#define LIBIS_SOURCE_H

#include <libis.h>
#include <stdbool.h>

struct LibisSource_ {
    // Read single byte from source into *c.
    // If end of file is reached *eof sets to true and *c sets to '\0'.
    // Otherwise *eof sets to false and *c sets to the next byte in the source.
    LibisError (*read)(Libis *libis, LibisSource *source, bool *eof, char *c);

    // Free resources taken by a source.
    LibisError (*free)(Libis *libis, LibisSource *source);
};

#endif
