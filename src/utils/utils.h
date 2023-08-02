#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdio.h>
#include <assert.h>

#define LOG_UNUSED(val) ((void)(val))

#ifdef _DEBUG
    #define ASSERT(expr) assert(expr)

    #define LOG(format, ...)  fprintf(stdout, "[DEBUG] " format, ##__VA_ARGS__)
    #define LOGE(format, ...) fprintf(stderr, "[DEBUG] " format, ##__VA_ARGS__)

    #define CHECK_ARRAY_BOUNDS(index, size) ASSERT(index >= 0 && index < size && "Array index out of bounds!")

    #define DEBUG_MODE_PRINT LOG("DEBUG MODE\n")
#else
    #define DEBUG_MODE_PRINT
    #define ASSERT(expression)
    #define LOG(format, ...)
    #define LOGE(format, ...)
    #define CHECK_ARRAY_BOUNDS(index, size)
#endif

#endif // __UTILS_H__