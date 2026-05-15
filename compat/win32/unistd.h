/* POSIX <unistd.h> compatibility shim for MSVC */
#pragma once
#ifndef _COMPAT_UNISTD_H
#define _COMPAT_UNISTD_H

#include <sys/stat.h>  /* must come before we alias 'stat' */
#include <io.h>
#include <fcntl.h>
#include <process.h>
#include <direct.h>
#include <stdlib.h>
#include <stdint.h>

#ifdef _MSC_VER

#ifndef STDIN_FILENO
#  define STDIN_FILENO  0
#  define STDOUT_FILENO 1
#  define STDERR_FILENO 2
#endif

#ifndef _SSIZE_T_DEFINED
typedef intptr_t ssize_t;
#  define _SSIZE_T_DEFINED
#endif
typedef unsigned short mode_t;

/*
 * Map POSIX stat / fstat / lstat to MSVC 64-bit equivalents.
 * struct stat  →  struct _stat64
 * stat()       →  _stat64()
 * fstat()      →  _fstat64()
 * lstat()      →  _stat64()   (Windows has no symlink distinction here)
 */
#ifndef stat
#  define stat  _stat64
#endif
#ifndef fstat
#  define fstat _fstat64
#endif
#ifndef lstat
#  define lstat(path, buf) _stat64((path), (buf))
#endif

/* S_IS* predicates using MSVC _S_IF* constants */
#ifndef S_ISREG
#  define S_ISREG(m)  (((m) & _S_IFMT) == _S_IFREG)
#endif
#ifndef S_ISDIR
#  define S_ISDIR(m)  (((m) & _S_IFMT) == _S_IFDIR)
#endif
#ifndef S_ISLNK
#  define S_ISLNK(m)  0
#endif

/* POSIX mkdir(path, mode) — Windows _mkdir takes no mode argument */
#ifndef mkdir
#  define mkdir(path, mode) _mkdir(path)
#endif

/* realpath: MSVC equivalent is _fullpath */
static inline char *realpath(const char *path, char *resolved)
{
    return _fullpath(resolved, path, _MAX_PATH);
}

#endif /* _MSC_VER */
#endif /* _COMPAT_UNISTD_H */
