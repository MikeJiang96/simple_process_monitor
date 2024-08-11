#ifndef UTIL_DEBUG_H
#define UTIL_DEBUG_H

#include <errno.h>
#include <stdio.h>
#include <string.h>

#define DEBUG(...) \
    do {           \
    } while (0)

#define Log_error(...) fprintf(stderr, __VA_ARGS__)

#define STRERROR strerror(errno)

#endif
