#ifndef __UTILS_H__
#define __UTILS_H__

#include <assert.h>

#if 1
#define ASSERT(condition)                                                 \
    do                                                                    \
    {                                                                     \
        if (!(condition))                                                 \
        {                                                                 \
            fprintf(stderr, "Assertion failed: %s, file: %s, line: %d\n", \
                    #condition, __FILE__, __LINE__);                      \
            abort();                                                      \
        }                                                                 \
    } while (0)

#define CHECK_ARRAY_BOUNDS(index, size) assert(index >= 0 && index < size && "Array index out of bounds!")

#define LOG(fmt, ...)                                                            \
    do                                                                           \
    {                                                                            \
        fprintf(stdout, "[%s:%d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
    } while (0)

#else

#define ASSERT(condition)
#define CHECK_ARRAY_BOUNDS(index, size)
#define LOG(fmt, ...)

#endif

#endif // __UTILS_H__