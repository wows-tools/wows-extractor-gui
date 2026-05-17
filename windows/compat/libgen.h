/* POSIX <libgen.h> compatibility shim for MSVC (dirname / basename) */
#pragma once
#ifndef _COMPAT_LIBGEN_H
#define _COMPAT_LIBGEN_H

#ifdef _MSC_VER
#include <string.h>

/*
 * dirname: strip the last component of a path, modifying the string in place
 * and returning a pointer into it — same contract as POSIX dirname(3).
 */
static inline char *dirname(char *path) {
    static char dot[] = ".";
    if (!path || *path == '\0')
        return dot;

    /* strip trailing separators */
    size_t len = strlen(path);
    while (len > 1 && (path[len - 1] == '/' || path[len - 1] == '\\'))
        path[--len] = '\0';

    /* find the last separator */
    char *sep = NULL;
    for (char *p = path; *p; p++)
        if (*p == '/' || *p == '\\')
            sep = p;

    if (!sep)
        return dot; /* no separator: directory is "." */
    if (sep == path) {
        path[1] = '\0'; /* root: return "/" or "\" */
        return path;
    }
    *sep = '\0';
    return path;
}

/*
 * basename: return the last path component (pointer into the original string).
 */
static inline char *basename(char *path) {
    static char dot[] = ".";
    if (!path || *path == '\0')
        return dot;

    char *sep = NULL;
    for (char *p = path; *p; p++)
        if (*p == '/' || *p == '\\')
            sep = p;

    return sep ? sep + 1 : path;
}

#endif /* _MSC_VER */
#endif /* _COMPAT_LIBGEN_H */
