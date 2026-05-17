/* open_memstream() shim for MSVC.
 *
 * POSIX open_memstream() writes to a dynamically-growing malloc'd buffer and
 * sets *bufp / *sizep when the stream is fclose()'d.  MSVC has no equivalent.
 *
 * Strategy: back the stream with tmpfile(); intercept fclose() via macro to
 * harvest the written bytes into a malloc'd buffer before closing the temp
 * file.  Only force-include this header for source files that actually call
 * open_memstream — the fclose macro is local to those TUs.
 *
 * Uses #pragma push_macro/pop_macro to capture the real fclose into an inline
 * helper before the macro shadows the name, avoiding dllimport static-
 * initializer restrictions in MSVC.
 */
#pragma once
#ifdef _MSC_VER
#ifndef _COMPAT_OPEN_MEMSTREAM_H
#define _COMPAT_OPEN_MEMSTREAM_H

#include <stdio.h>
#include <stdlib.h>

typedef struct {
    FILE *f;
    char **bufp;
    size_t *szp;
} _omstream_slot_t;

#define _OMSTREAM_MAX 16
static _omstream_slot_t _omstream_slots[_OMSTREAM_MAX];
static int _omstream_count = 0;

/* Wrap the real fclose *before* the macro below shadows the name.
 * #pragma push_macro ensures we see the CRT fclose, not our own override. */
#pragma push_macro("fclose")
#undef fclose
static inline int _omstream_real_fclose(FILE *f) {
    return fclose(f);
}
#pragma pop_macro("fclose")

static inline FILE *open_memstream(char **bufp, size_t *sizep) {
    FILE *f = tmpfile();
    if (!f)
        return NULL;
    if (_omstream_count < _OMSTREAM_MAX) {
        _omstream_slots[_omstream_count].f = f;
        _omstream_slots[_omstream_count].bufp = bufp;
        _omstream_slots[_omstream_count].szp = sizep;
        _omstream_count++;
    }
    *bufp = NULL;
    *sizep = 0;
    return f;
}

static inline int _omstream_fclose(FILE *f) {
    int i;
    for (i = 0; i < _omstream_count; i++) {
        if (_omstream_slots[i].f != f)
            continue;
        long n = ftell(f);
        if (n > 0) {
            char *buf = (char *)malloc((size_t)n);
            if (buf) {
                rewind(f);
                (void)fread(buf, 1, (size_t)n, f);
                *_omstream_slots[i].bufp = buf;
                *_omstream_slots[i].szp = (size_t)n;
            }
        }
        _omstream_slots[i] = _omstream_slots[--_omstream_count];
        return _omstream_real_fclose(f);
    }
    return _omstream_real_fclose(f);
}

#define fclose _omstream_fclose

#endif /* _COMPAT_OPEN_MEMSTREAM_H */
#endif /* _MSC_VER */
