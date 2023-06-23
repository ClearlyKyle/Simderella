#ifndef __JS_H__
#define __JS_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include <windows.h>

/* Define this before the header to include the function bodies */
// #define  JOB_SYHSTEM_IMPLEMENTATION

#define NUM_OF_THREADS     7 // Minus 1 for the main thread
#define MAX_NUMBER_OF_JOBS 4096
//#define DEBUG

typedef struct
{
    void (*function)(void *);
    void *arguments;
} job_t;

typedef struct
{
    int logical_thread_index;
} thread_info_t;

typedef struct
{
    job_t jobs[MAX_NUMBER_OF_JOBS];
    size_t volatile read_index;
    size_t volatile write_index;
} job_queue_t;

typedef struct
{
    job_queue_t job_queue;
    void       *job_semaphore;

    size_t volatile number_of_jobs;
    size_t volatile number_of_jobs_complete;

    size_t        number_of_threads;
    thread_info_t info[NUM_OF_THREADS];
} job_system_t;

extern job_system_t *JOB_STATE;

// Job functions
void jobs_init(void);
void jobs_shutdown(void);
bool job_submit(job_t job);
void jobs_complete_all_work(void);

// Thread handling functions
extern int     Platform_ReleaseSemaphore(void *semaphore);
extern int     Platform_WaitForSingleObject(void *semaphore);
extern int32_t Platform_InterlockedCompareExchange(int32_t *dest, int32_t exchange, int32_t compare);
extern int32_t Platform_InterlockedIncrement(int32_t *addend);
extern int32_t Platform_InterlockedDecrement(int32_t *addend);
extern void    Platform_CloseHandle(void *handle);
extern void   *Platform_create_semaphore(size_t initial_count, size_t maximum_count);

#ifdef JOB_SYHSTEM_IMPLEMENTATION

static bool _Do_Work_Queue_Entry(int worker_thread_id)
{
#ifndef DEBUG
    worker_thread_id = 0; // Remove the MSVC Warning
#endif
    job_queue_t *job_queue = &JOB_STATE->job_queue;

    // Read the current read position
    const size_t current_read_index = job_queue->read_index;
    const size_t new_read_index     = (current_read_index + 1) % MAX_NUMBER_OF_JOBS;

    // Check if the buffer is empty
    if (current_read_index != job_queue->write_index)
    {
        const size_t index = Platform_InterlockedCompareExchange((int32_t *)&job_queue->read_index,
                                                                 (int32_t)new_read_index,
                                                                 (int32_t)current_read_index);
        if (index == current_read_index)
        {
            job_t job = job_queue->jobs[index];
#ifdef DEBUG
            printf("Thread %d is doing work\n", worker_thread_id);
            if (job.function)
                job.function(job.arguments);
            printf("Thread %d has finished doing work\n", worker_thread_id);
#else
            if (job.function)
                job.function(job.arguments);
#endif
            Platform_InterlockedIncrement((int32_t *)&JOB_STATE->number_of_jobs_complete);
        } // else, another thread has beat me here and I should not do anything
    }
    else
    {
        return true; // Sleep
    }
    /* For some reason this can cause perfance drop if set to true... */
    return false; // Dont sleep the thread
}

DWORD WINAPI WorkerThread(LPVOID lpParam)
{
    thread_info_t *thread_info = (thread_info_t *)(lpParam);

    while (true)
    {
        if (_Do_Work_Queue_Entry(thread_info->logical_thread_index))
        {
            Platform_WaitForSingleObject(JOB_STATE->job_semaphore);
        }
    }

    return 0;
}

void jobs_complete_all_work(void)
{
#ifdef DEBUG
    printf("BEFORE jobs_complete_all_work -> (job count: %zu)(complete: %zu)\n",
           JOB_STATE->number_of_jobs,
           JOB_STATE->number_of_jobs_complete);
#endif
    // fprintf(stderr, "BEFORE jobs_complete_all_work -> (job count: %zu)(complete: %zu)\n",
    //         JOB_STATE->number_of_jobs,
    //         JOB_STATE->number_of_jobs_complete);

    size_t num_jobs      = JOB_STATE->number_of_jobs;
    size_t complete_jobs = JOB_STATE->number_of_jobs_complete;
    do
    {
        _Do_Work_Queue_Entry(NUM_OF_THREADS);

        num_jobs      = JOB_STATE->number_of_jobs;
        complete_jobs = JOB_STATE->number_of_jobs_complete;
    } while (num_jobs != complete_jobs);

#ifdef DEBUG
    printf("AFTER jobs_complete_all_work -> (job count: %zu)(complete: %zu)\n",
           JOB_STATE->number_of_jobs,
           JOB_STATE->number_of_jobs_complete);
#endif

    JOB_STATE->number_of_jobs          = 0;
    JOB_STATE->number_of_jobs_complete = 0;
    // JOB_STATE->job_queue.read_index    = 0;
    // JOB_STATE->job_queue.write_index   = 0;
}

void jobs_init(void)
{
    JOB_STATE = malloc(sizeof(job_system_t));
    assert(JOB_STATE);

    memset(JOB_STATE, 0, sizeof(job_system_t));

    JOB_STATE->number_of_threads = NUM_OF_THREADS;

    // Initialize job semaphore with a count of 0, indicating no jobs are pending
    const LONG thread_count  = (LONG)NUM_OF_THREADS;
    const LONG initial_count = 0;

    JOB_STATE->job_semaphore = Platform_create_semaphore(initial_count, thread_count);

    memset(JOB_STATE->job_queue.jobs, 0, sizeof(job_t) * MAX_NUMBER_OF_JOBS);
    JOB_STATE->job_queue.read_index  = 0;
    JOB_STATE->job_queue.write_index = 0;

    for (int thread_index = 0; thread_index < NUM_OF_THREADS; thread_index++)
    {
        thread_info_t *thread_info        = JOB_STATE->info + thread_index;
        thread_info->logical_thread_index = thread_index;

        LPDWORD thread_id     = NULL;
        HANDLE  thread_handle = CreateThread(0, 0, WorkerThread, (void *)(thread_info), 0, thread_id);
        Platform_CloseHandle(thread_handle);
    }
}

void jobs_shutdown(void)
{
#ifdef DEBUG
    printf("jobs_shutdown -> (job count: %zu)(complete: %zu)\n",
           JOB_STATE->number_of_jobs,
           JOB_STATE->number_of_jobs_complete);
#endif
    // Release all worker threads
    for (int i = 0; i < JOB_STATE->number_of_threads; i++)
    {
        Platform_ReleaseSemaphore(JOB_STATE->job_semaphore);
    }

    // Clean up the job queue, semaphore, and mutex
    memset(JOB_STATE->job_queue.jobs, 0, sizeof(job_t) * MAX_NUMBER_OF_JOBS);

    Platform_CloseHandle(JOB_STATE->job_semaphore);
}

bool job_submit(job_t job)
{
    job_queue_t *queue = &JOB_STATE->job_queue;

    size_t current_write_index, new_write_index;

    do
    {
        current_write_index = queue->write_index;                             // Read the current write position
        new_write_index     = (current_write_index + 1) % MAX_NUMBER_OF_JOBS; // Calculate the new write position

        if (new_write_index == queue->read_index) // Check if the buffer is full
        {
            printf("JOBS ARE FULL\n");
            printf("queue->write_index : %zd, queue->read_index : %zd\n", queue->write_index, queue->read_index);
            _Do_Work_Queue_Entry(NUM_OF_THREADS);
        }

    } while (Platform_InterlockedCompareExchange((int32_t *)&queue->write_index,
                                                 (int32_t)new_write_index,
                                                 (int32_t)current_write_index) != current_write_index);

    queue->jobs[current_write_index] = job; // Add the item to the buffer

    Platform_InterlockedIncrement((int32_t *)&JOB_STATE->number_of_jobs);

    Platform_ReleaseSemaphore(JOB_STATE->job_semaphore); // signal the worker threads
    return true;
}

// Linux version using GCC built-in function
#if defined(__GNUC__)
#include <semaphore.h>

job_system_t *JOB_STATE = NULL;

static inline int Platform_ReleaseSemaphore(sem_t *semaphore)
{
#ifdef DEBUG
    int err = sem_post(sem);
    if (err == -1)
    {
        char errorMsg[1024];
        strerror_r(errno, errorMsg, sizeof(errorMsg));
        printf("sem_post failed with error: %s\n", errorMsg);
    }
#else
    return sem_post(semaphore);
#endif
}

static inline int Platform_WaitForSingleObject(sem_t *semaphore)
{
#ifdef DEBUG
    int err = sem_wait(sem);
    if (err == -1)
    {
        char errorMsg[1024];
        strerror_r(errno, errorMsg, sizeof(errorMsg));
        printf("sem_wait failed with error: %s\n", errorMsg);
    }
#else
    return sem_wait(semaphore);
#endif
}

static inline int32_t Platform_InterlockedCompareExchange(int32_t *dest, int32_t exchange, int32_t compare)
{
    return __sync_val_compare_and_swap(dest, compare, exchange);
}

static inline int32_t Platform_InterlockedIncrement(int32_t *addend)
{
    return __sync_add_and_fetch(addend, 1);
}

static inline int32_t Platform_InterlockedDecrement(int32_t *addend)
{
    return __sync_sub_and_fetch(addend, 1);
}

sem_t *Platform_create_semaphore(unsigned int initial_value, unsigned int maximum_value, const char *name, int flags, mode_t mode)
{
    sem_t *semaphore = sem_open(name, flags, mode, initial_value);
    if (semaphore == SEM_FAILED)
    {
        return NULL;
    }

    // Set the maximum value of the semaphore
    int set_value_result = sem_setvalue(semaphore, maximum_value);
    if (set_value_result == -1)
    {
        sem_close(semaphore);
        sem_unlink(name);
        return NULL;
    }

    return semaphore;
}

#elif defined(_WIN32)

job_system_t *JOB_STATE = NULL;

/**
 * Releases a semaphore object, increasing its count by one.
 *
 * @param semaphore A handle to the semaphore object.
 *
 * @return If the function succeeds, the return value is nonzero.
 *         If the function fails, the return value is zero.
 *
 * @remarks This function releases a semaphore object, increasing its count by
 *          one. It is implemented using the ReleaseSemaphore Windows API
 *          function. The function returns a nonzero value if the semaphore
 *          was released successfully, or zero if the function failed.
 *          In debug builds, it also logs an error message if
 *          ReleaseSemaphore fails.
 *
 * @throws This function does not throw any exceptions.
 */
inline int Platform_ReleaseSemaphore(void *semaphore)
{
#ifdef DEBUG
    const int result = ReleaseSemaphore((HANDLE)semaphore, 1, 0);
    if (!result)
    {
        DWORD error = GetLastError();
        LPVOID lpMsgBuf;
        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);
        fprintf(stderr, "ReleaseSemaphore %d: %s\n", error, (LPTSTR)lpMsgBuf);
        LocalFree(lpMsgBuf);
    }
    return result;
#else
    // If the function fails, the return value is zero
    return ReleaseSemaphore((HANDLE)semaphore, 1, 0);
#endif
}

/**
 * Waits for the specified object to enter the signaled state, or for the wait to be abandoned.
 *
 * @param handle A handle to the object to wait for.
 *
 * @return  Returns 0 if the function succeeds and the specified object is signaled.
 *          If the function fails or the wait is abandoned,
 *          the return value is an error code defined in winerror.h
 */
inline int Platform_WaitForSingleObject(void *handle)
{
#ifdef DEBUG
    DWORD result = WaitForSingleObject((HANDLE)handle, INFINITE);
    if (result != WAIT_OBJECT_0)
    {
        DWORD error = GetLastError();
        LPVOID lpMsgBuf;
        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);
        printf("WaitForSingleObject failed with error %d: %s", error, (char *)lpMsgBuf);
        return error;
    }
    return 0;
#else
    return WaitForSingleObject((HANDLE)handle, INFINITE);
#endif
}

/**
 * Atomically compares the value pointed to by dest with the value of compare,
 * and if they are equal, sets the value pointed to by dest to exchange.
 *
 * @param dest A pointer to the integer value to compare and potentially set.
 * @param exchange The value to set dest to if its current value equals compare.
 * @param compare The value to compare against the current value of dest.
 *
 * @return The previous value of the integer pointed to by dest.
 *
 * @remarks This function provides atomicity for operations on shared data in
 *          multi-threaded environments. It ensures that only one thread can
 *          modify the data at a time, preventing race conditions and other bugs.
 *          The function is implemented using the InterlockedCompareExchange
 *          Windows API function.
 *
 * @throws This function does not throw exceptions.
 */
inline int32_t Platform_InterlockedCompareExchange(int32_t *dest, int32_t exchange, int32_t compare)
{
    assert(dest);
    return InterlockedCompareExchange((volatile LONG *)dest, exchange, compare);
}

/**
 * Atomically increments the value pointed to by addend by one.
 *
 * @param addend A pointer to the integer value to increment.
 *
 * @return The new value of the integer pointed to by addend.
 *
 * @remarks This function provides atomicity for operations on shared data in
 *          multi-threaded environments. It ensures that only one thread can
 *          modify the data at a time, preventing race conditions and other bugs.
 *          The function is implemented using the InterlockedIncrement
 *          Windows API function.
 *
 * @throws This function does not throw exceptions.
 */
inline int32_t Platform_InterlockedIncrement(int32_t *addend)
{
    assert(addend);
    return InterlockedIncrement((volatile LONG *)addend);
}

/**
 * Atomically decrements the value pointed to by addend by one.
 *
 * @param addend A pointer to the integer value to decrement.
 *
 * @return The new value of the integer pointed to by addend.
 *
 * @remarks This function provides atomicity for operations on shared data in
 *          multi-threaded environments. It ensures that only one thread can
 *          modify the data at a time, preventing race conditions and other bugs.
 *          The function is implemented using the InterlockedDecrement
 *          Windows API function.
 *
 * @throws This function does not throw exceptions.
 */
inline int32_t Platform_InterlockedDecrement(int32_t *addend)
{
    assert(addend);
    return InterlockedDecrement((volatile LONG *)addend);
}

/**
 * Closes a handle to a kernel object.
 *
 * @param handle A handle to the kernel object to close.
 *
 * @remarks This function closes a handle to a kernel object, such as a file or
 *          thread. It is implemented using the CloseHandle Windows API
 *          function. In debug builds, it also logs an error message if
 *          CloseHandle fails.
 *
 * @throws This function does not throw exceptions.
 */
inline void Platform_CloseHandle(void *handle)
{
    assert(handle);
#ifdef DEBUG
    DWORD result = CloseHandle((HANDLE)handle);
    if (result == 0)
    {
        DWORD error = GetLastError();
        LPVOID lpMsgBuf;
        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);
        printf("CloseHandle failed with error %d: %s", error, (char *)lpMsgBuf);
    }
#else
    CloseHandle((HANDLE)handle);
#endif
}

/**
 * Creates a new semaphore with the specified initial and maximum counts.
 *
 * @param initial_count The initial count for the semaphore.
 * @param maximum_count The maximum count for the semaphore.
 *
 * @return A handle to the newly created semaphore.
 *
 * @remarks This function creates a new semaphore with the specified initial
 *          and maximum counts. It is implemented using the CreateSemaphoreEx
 *          Windows API function. The returned handle can be used to wait on or
 *          signal the semaphore.
 *
 * @throws This function throws an exception if the semaphore cannot be created.
 *
 * @warning The caller is responsible for closing the handle to the semaphore
 *          using the Platform_CloseHandle function when it is no longer needed.
 */
inline void *Platform_create_semaphore(size_t initial_count, size_t maximum_count)
{
    return CreateSemaphoreEx(0, (LONG)initial_count, (LONG)maximum_count, 0, 0, SEMAPHORE_ALL_ACCESS);
}

#else
#error Certain functions are not supported on this platform
#endif
#endif // JOB_SYHSTEM_IMPLEMENTATION

#endif // __JS_H__