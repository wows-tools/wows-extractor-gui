/* <linux/limits.h> compatibility shim for MSVC */
#pragma once
#ifndef _COMPAT_LINUX_LIMITS_H
#define _COMPAT_LINUX_LIMITS_H

#include <limits.h>  /* MSVC provides PATH_MAX via <limits.h> or <stdlib.h> */

#ifndef PATH_MAX
#  define PATH_MAX  _MAX_PATH   /* typically 260 on Windows */
#endif
#ifndef NAME_MAX
#  define NAME_MAX  255
#endif

#endif /* _COMPAT_LINUX_LIMITS_H */
