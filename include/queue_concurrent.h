#ifndef QUEUE_CONCURRENT_H
#define QUEUE_CONCURRENT_H
#include <time.h>
#include <unistd.h>

/* DATA */

#define QUEUE_C_UNLIMITED 0 // Unlimited capacity for queue

/**
 * @brief A pointer to a user-defined free function.  This is used to free
 *        memory allocated for queue data.  For simple data types, this is
 *        just a pointer to the standard free function.  More complex structs
 *        stored in queues may require a function that calls free on multiple
 *        components.
 *
 */
typedef void (*FREE_F)(void *);

typedef struct queue_c_t queue_c_t;

/* FUNCTIONS */

/**
 * @brief Create a new concurrent queue.
 *
 * This function allocates memory for a new queue object. This queue object
 * is designed to be thread safe. It can be used in a single threaded
 * application, but it is recommended to use a queue_t object instead.
 *
 * A capacity of 0 will create a queue with unlimited capacity.
 *
 * Possible errno values:
 * - ENOMEM: malloc failed to allocate memory
 *
 * @param capacity max number of nodes the queue will hold
 * @param customfree pointer to user defined free function
 * @note if the user passes in NULL, the queue will not free the data
 * @param err pointer to error code
 * @returns pointer to allocated queue on success or NULL on failure
 */
queue_c_t *queue_c_init(size_t capacity, FREE_F customfree, int *err);

/**
 * @brief Check if queue is full.
 *
 * If the queue has unlimited capacity, this function will always return 0.
 *
 * @param queue pointer queue object
 * @return 0 if queue is not full, positive if full, -1 if queue is NULL
 */
int queue_c_is_full(queue_c_t *queue);

/**
 * @brief Wait for queue to be full.
 *
 * This function will block until the queue is full. If the queue is already
 * full, this function will return immediately. Intended to be used in thread
 * based applications. Upon successful return, the queue will be in a locked
 * state so that changes to the queue on other threads will block. The queue
 * must then be unlocked by calling queue_c_unlock().
 *
 * If the queue is NULL or has been destroyed before this function was called,
 * EINVAL will be returned. If the queue is destroyed while this function is
 * waiting, EINTR will be returned. If the wait has been cancelled but the queue
 * is not being destroyed, EAGAIN will be returned. If the queue is in a
 * locked state from one of the other waiting/locking functions from this
 * thread, EDEADLK will be returned; in this situation, the state of the lock
 * will not be changed and the condition is not evaluated. If the queue has
 * unlimited capacity, this function will return ENOTSUP.
 *
 * @param queue pointer queue object
 * @return int 0 on success, non-zero on failure
 */
int queue_c_wait_for_full(queue_c_t *queue);

/**
 * @brief Wait for queue to be full or timeout.
 *
 * This function will block until the queue is full or the timeout is reached.
 * If the queue is already full, this function will return immediately. Intended
 * to be used in thread based applications. Upon successful return, the queue
 * will be in a locked state so that changes to the queue on other threads will
 * block. The queue must be unlocked by calling queue_c_unlock().
 *
 * If timeout is 0, this function will behave the same as
 * queue_c_wait_for_full().
 *
 * If the queue is NULL or has been destroyed before this function was called,
 * EINVAL will be returned, EINVAL will be returned. If the queue is destroyed
 * while this function is waiting, EINTR will be returned. If the wait has been
 * cancelled but the queue is not being destroyed, EAGAIN will be returned. If
 * the timeout is reached, ETIMEDOUT will be returned. If the queue is in a
 * locked state from one of the other waiting/locking functions from this
 * thread, EDEADLK will be returned; in this situation, the state of the lock
 * will not be changed and the condition is not evaluated. If the queue has
 * unlimited capacity, this function will return ENOTSUP.
 *
 * @param queue pointer queue object
 * @param timeout time in seconds to wait for queue to be full
 * @return int 0 on success, non-zero on failure
 */
int queue_c_timed_wait_for_full(queue_c_t *queue, time_t timeout);

/**
 * @brief Wait for queue to not be full.
 *
 * This function will block until the queue is not full. If the queue is not
 * full, this function will return immediately. Intended to be used in thread
 * based applications. Upon successful return, the queue will be in a locked
 * state so that changes to the queue on other threads will block. The queue
 * must then be unlocked by calling queue_c_unlock().
 *
 * If the queue is NULL or has been destroyed before this function was called,
 * EINVAL will be returned, EINVAL will be returned. If the queue is destroyed
 * while this function is waiting, EINTR will be returned. If the wait has been
 * cancelled but the queue is not being destroyed, EAGAIN will be returned. If
 * the queue is in a locked state from one of the other waiting/locking
 * functions from this thread, EDEADLK will be returned; in this situation, the
 * state of the lock will not be changed and the condition is not evaluated. If
 * the queue has unlimited capacity, this function will return ENOTSUP.
 *
 * @param queue pointer queue object
 * @return int 0 on success, non-zero on failure
 */
int queue_c_wait_for_not_full(queue_c_t *queue);

/**
 * @brief Wait for queue to not be full or timeout.
 *
 * This function will block until the queue is not full or the timeout is
 * reached. If the queue is not full, this function will return immediately.
 * Intended to be used in thread based applications. Upon successful return, the
 * queue will be in a locked state so that changes to the queue on other threads
 * will block. The queue must then be unlocked by calling queue_c_unlock().
 *
 * If timeout is 0, this function will behave the same as
 * queue_c_wait_for_not_full().
 *
 * If the queue is NULL or has been destroyed before this function was called,
 * EINVAL will be returned, EINVAL will be returned. If the queue is destroyed
 * while this function is waiting, EINTR will be returned. If the wait has been
 * cancelled but the queue is not being destroyed, EAGAIN will be returned. If
 * the timeout is reached, ETIMEDOUT will be returned. If the queue is in a
 * locked state from one of the other waiting/locking functions from this
 * thread, EDEADLK will be returned; in this situation, the state of the lock
 * will not be changed and the condition is not evaluated. If the queue has
 * unlimited capacity, this function will return ENOTSUP.
 *
 * @param queue pointer queue object
 * @param timeout time in seconds to wait for queue to not be full
 * @return int 0 on success, non-zero on failure
 */
int queue_c_timed_wait_for_not_full(queue_c_t *queue, time_t timeout);

/**
 * @brief Check if queue is empty.
 *
 * @param queue pointer queue object
 * @return 0 if queue is not empty, positive if empty, -1 if queue is NULL
 */
int queue_c_is_empty(queue_c_t *queue);

/**
 * @brief Wait for queue to be empty.
 *
 * This function will block until the queue is empty. If the queue is already
 * empty, this function will return immediately. Intended to be used in thread
 * based applications. Upon successful return, the queue will be in a locked
 * state so that changes to the queue on other threads will block. The queue
 * must then be unlocked by calling queue_c_unlock().
 *
 * If the queue is NULL or has been destroyed before this function was called,
 * EINVAL will be returned, EINVAL will be returned. If the queue is destroyed
 * while this function is waiting, EINTR will be returned. If the wait has been
 * cancelled but the queue is not being destroyed, EAGAIN will be returned. If
 * the queue is in a locked state from one of the other waiting/locking
 * functions from this thread, EDEADLK will be returned; in this situation, the
 * state of the lock will not be changed and the condition is not evaluated.
 *
 * @param queue pointer queue object
 * @return int 0 on success, non-zero on failure
 */
int queue_c_wait_for_empty(queue_c_t *queue);

/**
 * @brief Wait for queue to be empty or timeout.
 *
 * This function will block until the queue is empty or the timeout is reached.
 * If the queue is already empty, this function will return immediately.
 * Intended to be used in thread based applications. Upon successful return, the
 * queue will be in a locked state so that changes to the queue on other threads
 * will block. The queue must then be unlocked by calling queue_c_unlock().
 *
 * If timeout is 0, this function will behave the same as
 * queue_c_wait_for_empty().
 *
 * If the queue is NULL or has been destroyed before this function was called,
 * EINVAL will be returned, EINVAL will be returned. If the queue is destroyed
 * while this function is waiting, EINTR will be returned. If the wait has been
 * cancelled but the queue is not being destroyed, EAGAIN will be returned. If
 * the timeout is reached, ETIMEDOUT will be returned. If the queue is in a
 * locked state from one of the other waiting/locking functions from this
 * thread, EDEADLK will be returned; in this situation, the state of the lock
 * will not be changed and the condition is not evaluated.
 *
 * @param queue pointer queue object
 * @param timeout time in seconds to wait for queue to be empty
 * @return int 0 on success, non-zero on failure
 */
int queue_c_timed_wait_for_empty(queue_c_t *queue, time_t timeout);

/**
 * @brief Wait for queue to not be empty.
 *
 * This function will block until the queue is not empty. If the queue is not
 * empty, this function will return immediately. Intended to be used in thread
 * based applications. Upon successful return, the queue will be in a locked
 * state so that changes to the queue on other threads will block. The queue
 * must then be unlocked by calling queue_c_unlock().
 *
 * If the queue is NULL or has been destroyed before this function was called,
 * EINVAL will be returned, EINVAL will be returned. If the queue is destroyed
 * while this function is waiting, EINTR will be returned. If the wait has been
 * cancelled but the queue is not being destroyed, EAGAIN will be returned. If
 * the queue is in a locked state from one of the other waiting/locking
 * functions from this thread, EDEADLK will be returned; in this situation, the
 * state of the lock will not be changed and the condition is not evaluated.
 *
 * @param queue pointer queue object
 * @return int 0 on success, non-zero on failure
 */
int queue_c_wait_for_not_empty(queue_c_t *queue);

/**
 * @brief Wait for queue to not be empty or timeout.
 *
 * This function will block until the queue is not empty or the timeout is
 * reached. If the queue is not empty, this function will return immediately.
 * Intended to be used in thread based applications. Upon successful return, the
 * queue will be in a locked state so that changes to the queue on other threads
 * will block. The queue must then be unlocked by calling queue_c_unlock().
 *
 * If timeout is 0, this function will behave the same as
 * queue_c_wait_for_not_empty().
 *
 * If the queue is NULL or has been destroyed before this function was called,
 * EINVAL will be returned, EINVAL will be returned. If the queue is destroyed
 * while this function is waiting, EINTR will be returned. If the wait has been
 * cancelled but the queue is not being destroyed, EAGAIN will be returned. If
 * the timeout is reached, ETIMEDOUT will be returned. If the queue is in a
 * locked state from one of the other waiting/locking functions from this
 * thread, EDEADLK will be returned; in this situation, the state of the lock
 * will not be changed and the condition is not evaluated.
 *
 * @param queue pointer queue object
 * @param timeout time in seconds to wait for queue to not be empty
 * @return int 0 on success, non-zero on failure
 */
int queue_c_timed_wait_for_not_empty(queue_c_t *queue, time_t timeout);

/**
 * @brief Cancel a wait on a queue.
 *
 * This function will cancel a wait on a queue on all threads. If no threads are
 * waiting on a condition, this function does nothing. For any threads that are
 * waiting, execution is resumed and the application should assume the condition
 * was not evaluated. Intended to be used in thread based applications.
 *
 * If the queue is NULL or destroyed before this function is called, EINVAL will
 * be returned.
 *
 * @param queue pointer queue object
 * @return int 0 on success, non-zero on failure
 */
int queue_c_cancel_wait(queue_c_t *queue);

/**
 * @brief Lock the queue.
 *
 * This function will lock the queue so that no other thread can modify it.
 * This, along with the other queue_c_wait() functions, allows for bulk
 * operations on the queue in the same thread. A call to this function should be
 * paired with a call to queue_c_unlock() to unlock the queue.
 *
 * If the queue is NULL, EINVAL will be returned. If the queue is destroyed
 * while this function is waiting, EINTR will be returned. If the queue is
 * destroyed before this function is called, EINVAL will be returned. If the
 * queue is in a locked state from one of the other waiting/locking functions
 * from this thread, EDEADLK will be returned; in this situation, the state
 * of the lock will not be changed.
 *
 * @param queue pointer queue object
 * @return int 0 on success, non-zero on failure
 */
int queue_c_lock(queue_c_t *queue);

/**
 * @brief Unlock the queue.
 *
 * This function will unlock the queue in the current thread. A call to this
 * function should be paired with a call to one of the lock functions to lock
 * the queue.
 *
 * If the queue is NULL, EINVAL will be returned. If the queue is destroyed
 * while this function is waiting, EINTR will be returned. If the queue is
 * destroyed before this function is called, EINVAL will be returned. If the
 * queue is not in a locked state from one of the other waiting/locking
 * functions from this thread, EPERM will be returned; in this situation, the
 * state of the lock will not be changed.
 *
 * @param queue pointer queue object
 * @return int 0 on success, non-zero on failure
 */
int queue_c_unlock(queue_c_t *queue);

/**
 * @brief Get the maximum capacity of the queue.
 *
 * If the queue has unlimited capacity, this function will return 0.
 *
 * @param queue pointer to queue to get capacity of
 * @return the capacity of the queue, -1 if queue is NULL
 */
ssize_t queue_c_capacity(queue_c_t *queue);

/**
 * @brief Get the current size of the queue.
 *
 * @param queue pointer to queue to get size of
 * @return the number of elements in the queue, -1 if queue is NULL
 */
ssize_t queue_c_size(queue_c_t *queue);

/**
 * @brief Push a new node into the queue.
 *
 * This function will attempt to obtain a lock on the queue. If the queue is
 * already locked on this thread from a previous call to a wait/lock function,
 * this function will continue operation and leave the queue locked when it is
 * done.
 *
 * A successful return of this function will trigger the condition variable for
 * threads waiting for the queue to not be empty, and potentially the variable
 * for full.
 *
 * If queue is NULL, EINVAL is returned. If the queue is full, EOVERFLOW is
 * returned. If malloc fails, ENOMEM is returned. If the queue is destroyed
 * while this function is waiting to lock, EINTR will be returned.
 *
 * @param queue pointer to queue pointer to push the node into
 * @param data data to be pushed into node
 * @return the 0 on success, non-zero on failure
 */
int queue_c_enqueue(queue_c_t *queue, void *data);

/**
 * @brief Pop the front node out of the queue.
 *
 * This function will attempt to obtain a lock on the queue. If the queue is
 * already locked on this thread from a previous call to a wait/lock function,
 * this function will continue operation and leave the queue locked when it is
 * done.
 *
 * A successful return of this function will trigger the condition variable for
 * threads waiting for the queue to not be full, and potentially the variable
 * for empty.
 *
 * Possible errno values:
 * - EINVAL: queue is NULL
 * - EINTR: queue is destroyed while waiting to lock
 *
 * @param queue pointer to queue pointer to pop the node off of
 * @param err pointer to error code
 * @return the 0 on success, NULL on failure
 */
void *queue_c_dequeue(queue_c_t *queue, int *err);

/**
 * @brief Get the data from the node at the front of the queue without popping.
 *
 * If queue is NULL, NULL is returned.
 *
 * @param queue pointer to queue pointer to peek
 * @return the pointer to the head on success, NULL on error
 */
void *queue_c_peek(queue_c_t *queue);

/**
 * @brief Clear all nodes out of a queue.
 *
 * This function will attempt to obtain a lock on the queue. If the queue is
 * already locked on this thread from a previous call to a wait/lock function,
 * this function will continue operation and leave the queue locked when it is
 * done.
 *
 * A successful return of this function will trigger the condition variables for
 * threads waiting for the queue to not be empty.
 *
 * If queue is NULL, EINVAL is returned. If the queue is destroyed while this
 * function is waiting to lock, EINTR will be returned.
 *
 * @param queue pointer to queue pointer to clear out
 * @return the 0 on success, non-zero on error
 */
int queue_c_clear(queue_c_t *queue);

/**
 * @brief Delete a queue.
 *
 * This function will attempt to obtain a lock on the queue. If the queue is
 * already locked on this thread from a previous call to a wait/lock function,
 * this function will proceed with destroying the queue. It will then signal
 * other threads about the destruction.
 *
 * Possible errno values:
 * - EINVAL: queue is NULL or destroyed before this function is called
 * - EINTR: queue is destroyed while waiting to lock
 *
 * @param queue_c_addr pointer to address of queue to be destroyed
 * @return the 0 on success, non-zero on error
 */
int queue_c_destroy(queue_c_t **queue_addr);

#endif /* QUEUE_CONCURRENT_H */
