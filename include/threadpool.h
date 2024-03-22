#ifndef THREADPOOL_H
#define THREADPOOL_H
#include <stddef.h>
#include <time.h>

/* DATA */

enum defaults_limits {
    DEFAULT_THREADS = 4, // default number of threads
    MAX_THREADS = 64,    // maximum number of threads
    DEFAULT_QUEUE = 16,  // default queue size
    DEFAULT_WAIT = 10,   // default wait time for blocking calls (in seconds)
};
enum shutdown_flags {
    SHUTDOWN_GRACEFUL = 1, // wait for all tasks to finish and queue to empty
    SHUTDOWN_FORCEFUL = 2, // cancel all threads immediately
};

/**
 * @brief Function pointer for the routine to be executed by the threadpool.
 *
 * The two arguments passed to the routine are optional. If the routine does
 * not require any arguments, the arguments can be ignored. Although not
 * required, the second argument is typically used to pass an output parameter
 * to the routine.
 *
 * @param arg The argument to be passed to the routine.
 * @param arg2 The second argument to be passed to the routine.
 */
typedef void (*ROUTINE)(void *arg, void *arg2);

typedef struct threadpool_attr_t threadpool_attr_t;

typedef struct threadpool_t threadpool_t;

/* attribute flags */

enum {
    CANCEL_DEFERRED, // cancel threads at cancellation points
    CANCEL_ASYNC,    // allow asynchronous cancellation
};
enum {
    TIMED_WAIT_DISABLED, // wait indefinitely when blocking
    TIMED_WAIT_ENABLED,  // use timeout when blocking
};
enum {
    BLOCK_ON_ADD_DISABLED, // do not block when adding to full queue
    BLOCK_ON_ADD_ENABLED,  // block when adding to full queue
};

/* FUNCTIONS */

/**
 * @brief Create a new threadpool.
 *
 * The threadpool will be created with the given attributes. If attr is NULL,
 * the default attributes will be used.
 *
 * Possible error codes:
 * ENOMEM - memory allocation failed
 * EAGAIN - thread creation failed
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
 * return EAGAIN (default behavior). Otherwise, the function will block until
 * there is room in the queue.
 *
 * If an error occurs while adding the task to the queue, the function will
 * return ENOMEM. If either pool or action is NULL, the function will return
 * EINVAL. If TIMED_WAIT is enabled, the default timeout will be used and the
 * function will return ETIMEDOUT if the timeout elapses.
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
 * If an error occurs while adding the task to the queue, the function will
 * return ENOMEM. If either pool or action are NULL or timeout is less than or
 * equal to 0 the function will return EINVAL. The function will return
 * ETIMEDOUT if the timeout elapses.
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
 * @brief Wait for all tasks in the threadpool to finish.
 *
 * If pool is NULL, the function will return EINVAL. If TIMED_WAIT_ENABLED is
 * set, the function will return ETIMEDOUT if the default timeout elapses.
 * Otherwise, the function will block until all tasks in the threadpool have
 * finished (default behavior).
 *
 * @param pool The threadpool to wait for.
 * @return int 0 on success, non-zero on failure.
 */
int threadpool_wait(threadpool_t *pool);

/**
 * @brief Wait for all tasks in the threadpool to finish.
 *
 * If pool is NULL, the function will return EINVAL. The TIMED_WAIT flag is
 * ignored and the function will return ETIMEDOUT if the specified seconds
 * elapse.
 *
 * @param pool The threadpool to wait for.
 * @param timeout The number of seconds to wait.
 * @return int 0 on success, non-zero on failure.
 */
int threadpool_timed_wait(threadpool_t *pool, time_t timeout);

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

/**
 * @brief Initialize a threadpool attribute object.
 *
 * The threadpool attribute object will be initialized with the default values.
 * This attribute object can be used to create a threadpool with custom
 * attributes. The same attribute object can be used to create multiple
 * threadpools. After the threadpool is created, the attribute object can be
 * destroyed, and modifying it will not affect the threadpool.
 *
 * If an error occurs while allocating memory for the attribute object, the
 * function will set errno to ENOMEM and return NULL.
 *
 * @return pointer to threadpool_attr_t.
 */
threadpool_attr_t *threadpool_attr_init(void);

/**
 * @brief Destroy a threadpool attribute object.
 *
 * The threadpool attribute object will be destroyed. If attr is NULL, the
 * function will return EINVAL.
 *
 * @param attr pointer to threadpool_attr_t
 * @return int 0 on success, non-zero on failure.
 */
int threadpool_attr_destroy(threadpool_attr_t *attr);

/**
 * @brief Set the cancellation type for the threadpool attribute object.
 *
 * The cancellation type will be set to either CANCEL_DEFERRED or CANCEL_ASYNC.
 * If attr is NULL or cancel_type is not CANCEL_DEFERRED or CANCEL_ASYNC, the
 * function will return EINVAL.
 *
 * @param attr pointer to threadpool_attr_t
 * @param cancel_type CANCEL_DEFERRED or CANCEL_ASYNC
 * @return int 0 on success, non-zero on failure.
 */
int threadpool_attr_set_cancel_type(threadpool_attr_t *attr, int cancel_type);

/**
 * @brief Get the cancellation type for the threadpool attribute object.
 *
 * The cancellation type will be returned in cancel_type. If attr or cancel_type
 * are NULL, the function will return EINVAL.
 *
 * @param attr pointer to threadpool_attr_t
 * @param cancel_type pointer to int
 * @return int 0 on success, non-zero on failure.
 */
int threadpool_attr_get_cancel_type(threadpool_attr_t *attr, int *cancel_type);

/**
 * @brief Set the timed wait flag for the threadpool attribute object.
 *
 * The timed wait flag will be set to either TIMED_WAIT_DISABLED or
 * TIMED_WAIT_ENABLED. Unless otherwise manually set, the actual wait time will
 * be set to DEFAULT_WAIT. Setting this flag does not modify any previous
 * changes to the actual wait time.
 *
 *  If attr is NULL or timed_wait is not TIMED_WAIT_DISABLED or
 * TIMED_WAIT_ENABLED, the function will return EINVAL.
 *
 * @param attr pointer to threadpool_attr_t
 * @param timed_wait TIMED_WAIT_DISABLED or TIMED_WAIT_ENABLED
 * @return int 0 on success, non-zero on failure.
 */
int threadpool_attr_set_timed_wait(threadpool_attr_t *attr, int timed_wait);

/**
 * @brief Get the timed wait flag for the threadpool attribute object.
 *
 * The timed wait flag will be returned in timed_wait. If attr or time_wait are
 * NULL, the function will return EINVAL.
 *
 * @param attr pointer to threadpool_attr_t
 * @param timed_wait pointer to int
 * @return int 0 on success, non-zero on failure.
 */
int threadpool_attr_get_timed_wait(threadpool_attr_t *attr, int *timed_wait);

/**
 * @brief Set the timeout for the threadpool attribute object.
 *
 * The timeout will be set to the given value. Note that this value is only used
 * if the timed wait flag is enabled.
 *
 * If attr is NULL or timeout is less than or equal to 0, the function will
 * return EINVAL.
 *
 * @param attr pointer to threadpool_attr_t
 * @param timeout time in seconds
 * @return int 0 on success, non-zero on failure.
 */
int threadpool_attr_set_timeout(threadpool_attr_t *attr, time_t timeout);

/**
 * @brief Get the timeout for the threadpool attribute object.
 *
 * The timeout will be returned in timeout. If attr or timeout are NULL, the
 * function will return EINVAL.
 *
 * @param attr pointer to threadpool_attr_t
 * @param timeout pointer to time_t
 * @return int 0 on success, non-zero on failure.
 */
int threadpool_attr_get_timeout(threadpool_attr_t *attr, time_t *timeout);

/**
 * @brief Set the block on add flag for the threadpool attribute object.
 *
 * The block on add flag will be set to either BLOCK_ON_ADD_DISABLED or
 * BLOCK_ON_ADD_ENABLED. If attr is NULL or block_on_add is not
 * BLOCK_ON_ADD_DISABLED or BLOCK_ON_ADD_ENABLED, the function will return
 * EINVAL.
 *
 * @param attr pointer to threadpool_attr_t
 * @param block_on_add BLOCK_ON_ADD_DISABLED or BLOCK_ON_ADD_ENABLED
 * @return int 0 on success, non-zero on failure.
 */
int threadpool_attr_set_block_on_add(threadpool_attr_t *attr, int block_on_add);

/**
 * @brief Get the block on add flag for the threadpool attribute object.
 *
 * The block on add flag will be returned in block_on_add. If attr or
 * block_on_add are NULL, the function will return EINVAL.
 *
 * @param attr pointer to threadpool_attr_t
 * @param block_on_add pointer to int
 * @return int 0 on success, non-zero on failure.
 */
int threadpool_attr_get_block_on_add(threadpool_attr_t *attr,
                                     int *block_on_add);

/**
 * @brief Set the number of threads for the threadpool attribute object.
 *
 * The number of threads will be set to the given value. If attr is NULL or
 * num_threads is less than or equal to 0, the function will return EINVAL. If
 * num_threads is greater than MAX_THREADS, the function will return EINVAL.
 *
 * @param attr pointer to threadpool_attr_t
 * @param num_threads number of threads
 * @return int 0 on success, non-zero on failure.
 */
int threadpool_attr_set_thread_count(threadpool_attr_t *attr,
                                     size_t num_threads);

/**
 * @brief Get the number of threads for the threadpool attribute object.
 *
 * The number of threads will be returned in num_threads. If attr or num_threads
 * are NULL, the function will return EINVAL.
 *
 * @param attr pointer to threadpool_attr_t
 * @param num_threads pointer to size_t
 * @return int 0 on success, non-zero on failure.
 */
int threadpool_attr_get_thread_count(threadpool_attr_t *attr,
                                     size_t *num_threads);

/**
 * @brief Set the queue size for the threadpool attribute object.
 *
 * The queue size will be set to the given value. If attr is NULL or queue_size
 * is less than or equal to 0, the function will return EINVAL.
 *
 * @param attr pointer to threadpool_attr_t
 * @param queue_size size of the queue
 * @return int 0 on success, non-zero on failure.
 */
int threadpool_attr_set_queue_size(threadpool_attr_t *attr, size_t queue_size);

/**
 * @brief Get the queue size for the threadpool attribute object.
 *
 * The queue size will be returned in queue_size. If attr or queue_size are
 * NULL, the function will return EINVAL.
 *
 * @param attr pointer to threadpool_attr_t
 * @param queue_size pointer to size_t
 * @return int 0 on success, non-zero on failure.
 */
int threadpool_attr_get_queue_size(threadpool_attr_t *attr, size_t *queue_size);
#endif /* THREADPOOL_H */
