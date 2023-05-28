#ifndef __TIMER_H__
#define __TIMER_H__

#include <windows.h>
#include <time.h>

#define time_this_funtion(a)                                                                 \
    do                                                                                       \
    {                                                                                        \
        clock_t start = clock();                                                             \
        a;                                                                                   \
        clock_t stop = clock();                                                              \
        printf("%s \t> Elapsed: %f seconds\n", #a, (double)(stop - start) / CLOCKS_PER_SEC); \
    } while (0)

typedef struct
{
    LARGE_INTEGER start;
    LARGE_INTEGER elapsed;
    LARGE_INTEGER perf_frequency;
    // char  *name;
} timer_t;

inline void Timer_Start(timer_t *const timer)
{
    QueryPerformanceFrequency(&timer->perf_frequency);
    QueryPerformanceCounter(&timer->start);
    timer->elapsed.QuadPart = 0;
}

inline timer_t Timer_Init_Start(void)
{
    timer_t t = {0};
    Timer_Start(&t);
    return t;
}

inline void Timer_Update(timer_t *const timer)
{
    LARGE_INTEGER new_time;
    QueryPerformanceCounter(&new_time);
    timer->elapsed.QuadPart = new_time.QuadPart - timer->start.QuadPart;
    timer->start.QuadPart   = new_time.QuadPart;
}

inline void Timer_Stop(timer_t *const timer)
{
    LARGE_INTEGER current_time;
    QueryPerformanceCounter(&current_time);
    timer->elapsed.QuadPart = current_time.QuadPart - timer->start.QuadPart;
}

inline double Timer_Get_Elapsed_Seconds(const timer_t *const timer)
{
    return (double)timer->elapsed.QuadPart / (double)timer->perf_frequency.QuadPart;
}

inline double Timer_Get_Elapsed_MS(const timer_t *const timer)
{
    return Timer_Get_Elapsed_Seconds(timer) * 1000.0;
}

#endif // __TIMER_H__