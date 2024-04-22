#ifndef THREADPOOL_H
#define THREADPOOL_H

#include "threadpool_attributes.h"

/* DATA */

typedef enum thread_status {
    STOPPED,    // thread is not running
    IDLE,       // thread is waiting for work
    RUNNING,    // thread is performing work
    BLOCKED,    // thread is blocked
    DESTROYING, // thread is being destroyed
} thread_status;

enum shutdown_flags {
    NO_SHUTDOWN,       // do not shutdown the threadpool
    SHUTDOWN_GRACEFUL, // wait for all tasks to finish and queue to empty
    SHUTDOWN_FORCEFUL, // cancel all threads immediately
};

/**
 * @brief Function pointer for the routine to be executed by the threadpool.
 *
 * The two arguments passed to the routine are optional. If the routine does
 * not require any arguments, the arguments can be ignored. Although not
 * required, the second argument is typically used to pass an output parameter
 * to the routine. The routine's return value will be stored by the threadpool,
 * and can be accessed later. The behavior of the threadpool changes based on
 * the block_on_error flag: if set to true, the executing thread will block
 * on a non-zero return value, and must be manually started again. Otherwise,
 * the threadpool will not block and continue to run, ignoring the error.
 *
 * @param arg The argument to be passed to the routine.
 * @param arg2 The second argument to be passed to the routine.
 * @return int The return value of the routine.
 */
typedef int (*ROUTINE)(void *arg, void *arg2);

typedef struct threadpool_t threadpool_t;

struct thread_info {
    size_t index;         // thread index
    ROUTINE action;       // routine to execute
    void *arg;            // routine argument
    void *arg2;           // second routine argument
    thread_status status; // thread status
    int error;            // error code
};

/* FUNCTIONS */

/**
 * @brief Create a new threadpool.
 *
 * The threadpool will be created with the given attributes. If attr is NULL,
 * the default attributes will be used.
 *
 * If the THREAD_CREATE_STRICT attribute is set, the function will require
 * all threads be successfully created before returning. Otherwise, threads
 * will be created as they are needed.
 *
 * Possible error codes:
 *      ENOMEM: memory allocation failed
 *      EAGAIN: THREAD_CREATE_STRICT is set and thread creation failed
 *
 * @param attr The threadpool attribute object.
 * @param err Where to store the error code.
 * @return threadpool_t* A pointer to the new threadpool, or NULL on failure.
 */
threadpool_t *threadpool_create(threadpool_attr_t *attr, int *err);

/**
 * @brief Add a new task to the threadpool.
 *
 * The task will be added to the threadpool's queue. If the queue is full and
 * BLOCK_ON_ADD is not set, the task will not be added and the function will
 * return EOVERFLOW (default behavior). Otherwise, the function will block until
 * there is room in the queue.
 *
 * If THREAD_CREATE_LAZY is set on the threadpool, the function will attempt
 * to create a new thread if there are no available threads to execute the task.
 * In this case, since the check for the thread is done after the task is added
 * to the queue, even if the thread creation fails the task will still be added
 * to the queue.
 *
 * Possible error codes:
 *      ENOMEM: memory allocation failed
 *      EOVERFLOW: queue is full and BLOCK_ON_ADD is not set
 *      EINVAL: pool or action is NULL
 *      ETIMEDOUT: TIMED_WAIT is enabled and the timeout elapses
 *      EAGAIN: THREAD_CREATE_LAZY is set and thread creation failed
 *
 * @param pool The threadpool to add the task to.
 * @param action The routine to be executed by the threadpool.
 * @param arg The argument to be passed to the routine.
 * @param arg2 The second argument to be passed to the routine.
 * @return int 0 on success, non-zero on failure.
 */
int threadpool_add_work(threadpool_t *pool, ROUTINE action, void *arg,
                        void *arg2);

/**
 * @brief Add a new task to the threadpool.
 *
 * The task will be added to the threadpool's queue. If the queue is full, the
 * function will block until there is room in the queue, or the given timeout
 * has elapsed. This function ignores the BLOCK_ON_ADD and TIMED_WAIT flags set
 * on the threadpool.
 *
 * If THREAD_CREATE_LAZY is set on the threadpool, the function will attempt
 * to create a new thread if there are no available threads to execute the task.
 * In this case, since the check for the thread is done after the task is added
 * to the queue, even if the thread creation fails the task will still be added
 * to the queue.
 *
 * Possible error codes:
 *      ENOMEM: memory allocation failed
 *      EINVAL: pool or action is NULL or timeout is negative
 *      ETIMEDOUT: the timeout elapses
 *      EAGAIN: THREAD_CREATE_LAZY is set and thread creation failed
 *
 * @param pool The threadpool to add the task to.
 * @param action The routine to be executed by the threadpool.
 * @param arg The argument to be passed to the routine.
 * @param arg2 The second argument to be passed to the routine.
 * @param timeout The number of seconds to wait.
 * @return int 0 on success, non-zero on failure.
 */
int threadpool_timed_add_work(threadpool_t *pool, ROUTINE action, void *arg,
                              void *arg2, time_t timeout);

/**
 * @brief Get the information of a thread in the threadpool.
 *
 * The function will return the information of the thread with the given
 * thread_id. The information returned is just a snapshot of the thread's
 * current state; the thread may change state after the function returns.
 *
 * Possible error codes:
 *      EINVAL: pool is NULL
 *      ENOENT: thread_id is not valid
 *
 * @param pool The threadpool to get the thread information from.
 * @param thread_id The id of the thread to get the information of.
 * @param info The struct to store the thread information.
 * @return int 0 on success, non-zero on failure.
 */
int threadpool_thread_status(threadpool_t *pool, size_t thread_id,
                             struct thread_info *info);

/**
 * @brief Get the information of all threads in the threadpool.
 *
 * The function will return the information of all threads in the threadpool.
 * The information returned is just a snapshot of the threads' current state;
 * the threads may change state after the function returns.

 * The array returned is internally maintained by the threadpool. Repeated
 * calls to this function will return the same array, but the contents are
 * only updated when the function is called. If the address of the array is not
 * needed, the function can be called with NULL. The array must not be freed by
 * the caller.
 *
 * Possible error codes:
 *      EINVAL: pool is NULL
 *
 * @param pool The threadpool to get the thread information from.
 * @param info_arr The array to store the thread information.
 * @return int 0 on success, non-zero on failure.
 */
int threadpool_thread_status_all(threadpool_t *pool,
                                 struct thread_info **info_arr);

/**
 * @brief Restart a thread in the threadpool.
 *
 * The threadpool will attempt to restart the thread with the given thread_id.
 * Only STOPPED and BLOCKED threads can be restarted. Any recorded errors
 * on BLOCKED threads will be cleared.
 *
 * Possible error codes:
 *      EINVAL: pool is NULL
 *      ENOENT: thread_id is not valid
 *      EAGAIN: Insufficient resources to create another thread.
 *      EALREADY: thread is already running
 *
 * @param pool The threadpool to restart the thread in.
 * @param thread_id The id of the thread to restart.
 * @return int 0 on success, non-zero on failure.
 */
int threadpool_restart_thread(threadpool_t *pool, size_t thread_id);

/**
 * @brief Restart all threads in the threadpool.
 *
 * The threadpool will attempt to restart all threads in the threadpool. If
 * THREAD_CREATE_STRICT is set, both STOPPED and BLOCKED threads can be
 * restarted. If THREAD_CREATE_LAZY is set, only BLOCKED threads can be
 * restarted. Any recorded errors on BLOCKED threads will be cleared.
 *
 * If the THREAD_CREATE_STRICT attribute is set, the function will fail if there
 * are insufficient resources to create another thread.
 *
 * Possible error codes:
 *      EINVAL: pool is NULL
 *      EAGAIN: THREAD_CREATE_STRICT is set and thread creation failed
 *
 * @param pool The threadpool to restart the threads in.
 * @return int 0 on success, non-zero on failure.
 */
int threadpool_refresh(threadpool_t *pool);

/**
 * @brief Wait for all tasks in the threadpool to finish.
 *
 * Blocks until all tasks in the threadpool have finished executing. If the
 * TIMED_WAIT flag is set on the threadpool, the default timeout will be used.
 *
 * Possible error codes:
 *      EINVAL: pool is NULL
 *      ETIMEDOUT: TIMED_WAIT is enabled and the default timeout elapses
 *      EAGAIN: The wait was cancelled
 *
 * @param pool The threadpool to wait for.
 * @return int 0 on success, non-zero on failure.
 */
int threadpool_wait(threadpool_t *pool);

/**
 * @brief Wait for all tasks in the threadpool to finish.
 *
 * Blocks until all tasks in the threadpool have finished executing. The
 * TIMED_WAIT flag is ignored, and the given timeout will always be used.
 *
 * Possible error codes:
 *      EINVAL: pool is NULL or timeout is negative
 *      ETIMEDOUT: The given timeout elapses
 *      EAGAIN: The wait was cancelled
 *
 * @param pool The threadpool to wait for.
 * @param timeout The number of seconds to wait.
 * @return int 0 on success, non-zero on failure.
 */
int threadpool_timed_wait(threadpool_t *pool, time_t timeout);

/**
 * @brief Cancel a wait on the threadpool.
 *
 * This function will cancel a wait on the threadpool on all threads. If no
 * threads are in a waiting status, this function does nothing. For any
 * threads that are waiting, execution is resumed and the application should
 * assume the running threads have not completed all of their work.
 *
 * Possible error codes:
 *      EINVAL: pool is NULL
 *
 * @param pool The threadpool to cancel the wait on.
 * @return int 0 on success, non-zero on failure
 */
int threadpool_cancel_wait(threadpool_t *pool);

/**
 * @brief Signal the threadpool.
 *
 * The threadpool will be signaled with the given signal. The signal will be
 * sent to all running (not idle) threads in the threadpool.
 *
 * Possible error codes:
 *      EINVAL: pool is NULL or sig is not recognized
 *
 * @param pool The threadpool to signal.
 * @param sig The signal to send.
 * @return int 0 on success, non-zero on failure.
 */
int threadpool_signal_all(threadpool_t *pool, int sig);

/**
 * @brief Destroy the threadpool.
 *
 * The threadpool will be destroyed. If flag is SHUTDOWN_GRACEFUL, the function
 * will wait for all tasks to finish before destroying the threadpool. For
 * SHUTDOWN_FORCEFUL, the function will attempt to cancel all threads
 * immediately and destroy the threadpool.
 *
 * If ASYNC_CANCEL is set on the threadpool, the threads may be cancelled at any
 * time. Otherwise, the threads will be cancelled when they reach a cancellation
 * point (default behavior). Note that ASYNC_CANCEL only applies when the
 * SHUTDOWN_FORCE flag is given.
 *
 * If pool is NULL or any value other than SHUTDOWN_GRACEFUL or
 * SHUTDOWN_FORCEFUL are given, the function will return EINVAL.
 *
 * @param pool The threadpool to destroy.
 * @param flag The quit flag.
 * @return int 0 on success, non-zero on failure.
 */
int threadpool_destroy(threadpool_t *pool, int flag);

#endif /* THREADPOOL_H */
