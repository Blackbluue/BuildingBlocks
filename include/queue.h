#ifndef QUEUE_H
#define QUEUE_H

#include <unistd.h>

/* DATA */

#define QUEUE_UNLIMITED 0 // unlimited capacity queue

/**
 * @brief A pointer to a user-defined free function.  This is used to free
 *        memory allocated for queue data.  For simple data types, this is
 *        just a pointer to the standard free function.  More complex structs
 *        stored in queues may require a function that calls free on multiple
 *        components.
 *
 */
typedef void (*FREE_F)(void *);

/**
 * @brief A user-defined compare function.
 *
 * Positive return value indicates that the first argument is greater than the
 * second argument. Negative return value indicates that the first argument is
 * less than the second argument. Zero return value indicates that the first
 * argument is equal to the second argument.
 */
typedef int (*CMP_F)(const void *value_to_find, const void *node_data);

typedef struct queue_t queue_t;

/* FUNCTIONS */

/**
 * @brief Create a new queue.
 *
 * Creates a new queue with the specified capacity. The queue will be able to
 * hold capacity number of nodes; if any more nodes are attempted to be added,
 * the operation will fail. A capacity of 0 will be treated as unlimited
 * capacity. The queue will use the customfree function to free memory allocated
 * for data. If NULL is given, the queue will not manage given data memory. The
 * queue will use the compare function in look up operations. If NULL is given,
 * these operations will not be supported.
 *
 * If an error occurs, NULL will be returned.
 * Possible error codes are:
 * - ENOMEM: memory allocation failed
 *
 * @param capacity max number of nodes the queue will hold
 * @param customfree pointer to user defined free function
 * @note if the user passes in NULL, the queue will not free the data
 * @param compare pointer to user defined compare function
 * @param err pointer to integer to store error code
 * @returns pointer to allocated queue on success or NULL on failure
 */
queue_t *queue_init(size_t capacity, FREE_F customfree, CMP_F compare,
                    int *err);

/**
 * @brief Check if queue is full.
 *
 * If the queue's capacity is 0 (unlimited), this function will always return
 * false.
 *
 * If queue is NULL, -1 will be returned.
 *
 * @param queue pointer queue object
 * @return 0 if queue is not full, positive if full, -1 on error.
 */
int queue_is_full(const queue_t *queue);

/**
 * @brief Check if queue is empty.
 *
 * If queue is NULL, -1 will be returned.
 *
 * @param queue pointer queue object
 * @return 0 if queue is not empty, positive if empty, -1 on error.
 */
int queue_is_empty(const queue_t *queue);

/**
 * @brief Get the maximum capacity of the queue.
 *
 * A queue with 0 capacity is considered to have unlimited capacity.
 *
 * If queue is NULL, -1 will be returned.
 *
 * @param queue pointer to queue to get capacity of
 * @return the capacity of the queue, -1 on error.
 */
ssize_t queue_capacity(const queue_t *queue);

/**
 * @brief Get the current size of the queue.
 *
 * If queue is NULL, -1 will be returned.
 *
 * @param queue pointer to queue to get size of
 * @return the number of elements in the queue, -1 on error.
 */
ssize_t queue_size(const queue_t *queue);

/**
 * @brief Push a new node into the queue.
 *
 * If an error occurs, non-zero will be returned.
 * Possible error codes are:
 * - EINVAL: queue is NULL
 * - EOVERFLOW: queue is full
 * - ENOMEM: memory allocation failed
 *
 * @param queue pointer to queue pointer to push the node into
 * @param data data to be pushed into node
 * @return the 0 on success, non-zero on error.
 */
int queue_enqueue(queue_t *queue, void *data);

/**
 * @brief Pop the front node out of the queue.
 *
 * If queue is NULL, NULL will be returned.
 *
 * @param queue pointer to queue pointer to pop the node off of
 * @return the 0 on success, NULL on error.
 */
void *queue_dequeue(queue_t *queue);

/**
 * @brief Get the data from the node at a specific position in the queue.
 *
 * If queue is NULL, NULL will be returned.
 *
 * @param queue pointer to queue pointer to get the node from
 * @param position position in the queue to get the node from
 * @return the pointer to the node on success, NULL on failure
 */
void *queue_get(const queue_t *queue, size_t position);

/**
 * @brief Get the data from the node at the front of the queue without popping.
 *
 * If queue is NULL, NULL will be returned.
 *
 * @param queue pointer to queue pointer to peek
 * @return the pointer to the head on success, NULL on error.
 */
void *queue_peek(const queue_t *queue);

/**
 * @brief Remove the first occurrence of a value in the queue.
 *
 * If an error occurs, NULL will be returned.
 * Possible error codes are:
 * - EINVAL: queue is NULL
 * - ENOTSUP: queue does not support lookup operations
 *
 * @param queue pointer to queue to search
 * @param item_to_remove pointer to value to remove
 * @return pointer to the removed value, NULL if not found or on error
 */
void *queue_remove(queue_t *queue, void *item_to_remove, int *err);

/**
 * @brief Find the first occurrence of a value in the queue.
 *
 * If an error occurs, NULL will be returned.
 * Possible error codes are:
 * - EINVAL: queue is NULL
 * - ENOTSUP: queue does not support lookup operations
 *
 * @param queue pointer to queue to search
 * @param value_to_find pointer to value to find
 * @return pointer to the first occurrence of the value in the queue, NULL if
 *         not found or queue is NULL
 */
void *queue_find_first(const queue_t *queue, const void *value_to_find,
                       int *err);

/**
 * @brief Clear all nodes out of a queue.
 *
 * Possible error codes are:
 * - EINVAL: queue is NULL
 *
 * @param queue pointer to queue pointer to clear out
 * @return the 0 on success, non-zero on error.
 */
int queue_clear(queue_t *queue);

/**
 * @brief Delete a queue.
 *
 * Possible error codes are:
 * - EINVAL: queue is NULL
 *
 * @param queue_addr pointer to address of queue to be destroyed
 * @return the 0 on success, non-zero on error.
 */
int queue_destroy(queue_t **queue_addr);
#endif /* QUEUE_H */
