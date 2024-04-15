#ifndef THREADPOOL_ATTRIBUTES_H
#define THREADPOOL_ATTRIBUTES_H

#include "threadpool_types.h"
#include <time.h>

/* DATA */

#define DEFAULT_THREADS 4 // default number of threads
#define MAX_THREADS 64    // maximum number of threads
#define DEFAULT_QUEUE 16  // default queue size
#define DEFAULT_WAIT 10   // default wait time for blocking calls (in seconds)

/* attribute flags */

enum wait_type_flags {
    TIMED_WAIT_DISABLED, // wait indefinitely when blocking
    TIMED_WAIT_ENABLED,  // use timeout when blocking
};
enum block_on_add_flags {
    BLOCK_ON_ADD_DISABLED, // do not block when adding to full queue
    BLOCK_ON_ADD_ENABLED,  // block when adding to full queue
};
enum block_on_error_flags {
    BLOCK_ON_ERR_ENABLED,  // block when a routine returns an error
    BLOCK_ON_ERR_DISABLED, // do not block when a routine returns an error
};

/**
 * @brief Initialize a threadpool attribute object.
 *
 * The threadpool attribute object will be initialized with the default values.
 * This attribute object can be used to create a threadpool with custom
 * attributes. The same attribute object can be used to create multiple
 * threadpools. After the threadpool is created, the attribute object can be
 * destroyed, and modifying it will not affect the threadpool.
 *
 * This function will always return 0.
 *
 * @param attr pointer to attribute object
 * @return always returns 0.
 */
int threadpool_attr_init(threadpool_attr_t *attr);

/**
 * @brief Destroy a threadpool attribute object.
 *
 * Destroys a threadpool attribute object, which must not be reused until it is
 * reinitialized. This function always returns 0.
 *
 * @note This function currently does not do anything.
 *
 * @param attr pointer to threadpool_attr_t
 * @return always returns 0.
 */
int threadpool_attr_destroy(threadpool_attr_t *attr);

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
 * @brief Set the block on error flag for the threadpool attribute object.
 *
 * The block on error flag will be set to either BLOCK_ON_ERR_DISABLED or
 * BLOCK_ON_ERR_ENABLED. If enabled, a thread will block when a routine returns
 * a non-zero, until the error is cleared.
 *
 * Possible return values:
 * - EINVAL: attr is NULL
 * - ENOTSUP: block_on_err is not BLOCK_ON_ERR_DISABLED or BLOCK_ON_ERR_ENABLED
 *
 * @param attr pointer to threadpool_attr_t
 * @param block_on_err BLOCK_ON_ERR_DISABLED or BLOCK_ON_ERR_ENABLED
 * @return int 0 on success, non-zero on failure.
 */
int threadpool_attr_set_block_on_err(threadpool_attr_t *attr, int block_on_err);

/**
 * @brief Get the block on error flag for the threadpool attribute object.
 *
 *  If enabled, a thread will block when a routine returns a non-zero, until the
 * error is cleared.
 *
 * Possible return values:
 * - EINVAL: attr or block_on_err are NULL
 *
 * @param attr pointer to threadpool_attr_t
 * @param block_on_err pointer to hold the value of the flag
 * @return int 0 on success, non-zero on failure.
 */
int threadpool_attr_get_block_on_err(threadpool_attr_t *attr,
                                     int *block_on_err);

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

#endif /* THREADPOOL_ATTRIBUTES_H */
