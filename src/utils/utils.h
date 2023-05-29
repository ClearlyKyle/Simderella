#ifndef __UTILS_H__
#define __UTILS_H__

#if 1

#define DEBUG_MODE_PRINT printf("DEBUG MODE\n")

#define CHECK_ARRAY_BOUNDS(index, size) assert(index >= 0 && index < size && "Array index out of bounds!")

#define LOG(fmt, ...)                                                            \
    do                                                                           \
    {                                                                            \
        fprintf(stdout, "[%s:%d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
    } while (0)

#define LOGERR(fmt, ...)                                                         \
    do                                                                           \
    {                                                                            \
        fprintf(stderr, "[%s:%d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
    } while (0)

#else

#define DEBUG_MODE_PRINT
#define CHECK_ARRAY_BOUNDS(index, size)
#define LOG(fmt, ...)
#define LOGERR(fmt, ...)

#endif

#endif // __UTILS_H__