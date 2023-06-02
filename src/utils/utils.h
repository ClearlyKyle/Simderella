#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdio.h>
#include <assert.h>

#if 0

#define DEBUG_MODE_PRINT printf("DEBUG MODE\n")
#define ASSERT(expression)              assert(expression)
#define LOG(format, ...)                fprintf(stdout, "[DEBUG] " format, ##__VA_ARGS__)
#define LOGE(format, ...)               fprintf(stderr, "[ERROR] " format, ##__VA_ARGS__)

/**
 * @brief check the index is in the bounds of the array
 *
 * @param index, which represents the index you want to access or modify in the array
 * @param size, which represents the max size of the array (max element)
 */
#define CHECK_ARRAY_BOUNDS(index, size) assert(index >= 0 && index < size && "Array index out of bounds!")

#else

#define DEBUG_MODE_PRINT
#define ASSERT(expression)
#define LOG(format, ...)
#define LOGE(format, ...)
#define CHECK_ARRAY_BOUNDS(index, size)

#endif

#endif // __UTILS_H__