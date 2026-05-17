/* fmemopen() shim for MSVC — POSIX function not available on Windows.
 * Writes the buffer to a temporary file and returns it seeked to the start.
 * The caller is responsible for fclose()ing the returned FILE*.
 */
#pragma once
#ifdef _MSC_VER
#ifndef _COMPAT_FMEMOPEN_H
#define _COMPAT_FMEMOPEN_H

#include <stdio.h>
#include <stdlib.h>

static inline FILE *fmemopen(void *buf, size_t size, const char *mode) {
    (void)mode;
    FILE *f = tmpfile();
    if (!f)
        return NULL;
    if (size > 0 && fwrite(buf, 1, size, f) != size) {
        fclose(f);
        return NULL;
    }
    rewind(f);
    return f;
}

#endif /* _COMPAT_FMEMOPEN_H */
#endif /* _MSC_VER */
