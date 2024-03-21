#ifndef QUEUE_P_H
#define QUEUE_P_H

#include <unistd.h>

/* DATA */

#define QUEUE_P_UNLIMITED 0 // unlimited capacity

/**
 * @brief structure of a queue node
 *
 * @param data      void pointer to whatever data that queue points to
 * @param priority  priority to add to q; higher priority gets preference in q
 */
typedef struct queue_p_node_t {
    void *data;
    double priority;
} queue_p_node_t;

/**
 * @brief A pointer to a user-defined free function.  This is used to free
 *        memory allocated for queue data.  For simple data types, this is
 *        just a pointer to the standard free function.  More complex structs
 *        stored in queues may require a function that calls free on multiple
 *        components.
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

typedef struct queue_p_t queue_p_t;

/* FUNCTIONS */

/**
 * @brief Create a new queue.
 *
 * If capacity is 0, the queue will be unlimited. If capacity is greater than 0,
 * the queue will be limited to the given capacity. If free_f is NULL, then data
 * stored in the queue will not be free'd.
 *
 * On error, the function will return NULL.
 * Possible error codes are:
 * - EINVAL: compare function is NULL
 * - ENOMEM: memory allocation failed
 *
 * @param capacity max number of nodes the queue will hold
 * @param customfree pointer to a function to free queue data
 * @note if the customfree in NULL, the queue will not free the data
 * @param compare pointer to user defined compare function
 * @param err pointer to integer to store error code
 * @returns pointer to allocated priority queue on success, NULL on failure
 */
queue_p_t *queue_p_init(size_t capacity, FREE_F customfree, CMP_F compare,
                        int *err);

/**
 * @brief Check if queue is full.
 *
 * If the queue capacity is 0, the queue is unlimited and this function will
 * always return 0.
 *
 * If queue is NULL, this function will return -1
 *
 * @param queue pointer queue object
 * @return 0 if queue is not full, non-zero if queue is full, -1 on error
 */
int queue_p_is_full(const queue_p_t *queue);

/**
 * @brief Check if queue is empty.
 *
 * If queue is NULL, this function will return -1
 *
 * @param queue pointer queue object
 * @return 0 if queue is not empty, non-zero if queue is empty, -1 on error
 */
int queue_p_is_empty(const queue_p_t *queue);

/**
 * @brief Get the maximum capacity of the queue.
 *
 * If the queue capacity is 0, the queue is unlimited.
 *
 * If queue is NULL, this function will return -1
 *
 * @param queue pointer to queue to get capacity of
 * @return the capacity of the queue, -1 if queue is NULL
 */
ssize_t queue_p_capacity(const queue_p_t *queue);

/**
 * @brief Get the current size of the queue.
 *
 * If queue is NULL, this function will return -1
 *
 * @param queue pointer to queue to get size of
 * @return the number of elements in the queue, -1 on error
 */
ssize_t queue_p_size(const queue_p_t *queue);

/**
 * @brief Push a new node into the queue.
 *
 * Possible error codes are:
 * - EINVAL: queue is NULL
 * - ENOMEM: memory allocation failed
 * - EOVERFLOW: queue is full
 *
 * @param queue pointer to queue pointer to push the node into
 * @param data data to be pushed into node
 * @param priority of data
 * @return 0 on success, non-zero value on failure
 */
int queue_p_enqueue(queue_p_t *queue, void *data, double priority);

/**
 * @brief Pop the front node out of the queue.
 *
 * The returned structure contains information on the dequeued node. The
 * structure must be freed after use.
 *
 * If queue is NULL, this function will return NULL
 *
 * @param queue pointer to queue pointer to pop the node off of
 * @return pointer to popped queue node on success, NULL on failure
 */
queue_p_node_t *queue_p_dequeue(queue_p_t *queue);

/**
 * @brief Get the data from the node at a specific position in the queue.
 *
 * The position is based on the entire queue, irrespective of priority.
 *
 * If queue is NULL or position is out of range, this function will return NULL.
 *
 * @param queue pointer to queue pointer to get the node from
 * @param position position in the queue to get the node from
 * @return the pointer to the node on success, NULL on failure
 * @note do not free the returned pointer, it is still in the queue
 */
queue_p_node_t *queue_p_get(const queue_p_t *queue, size_t position);

/**
 * @brief Get the data from the node at a specific position in the queue.
 *
 * This version of queue_p_get will return the node with the given position
 * in relation to the priority of the node. For example, if the position is
 * 2 and the priority is 1, the function will return the node with position
 * 2 in the queue in relation to all the other nodes with priority 1.
 *
 * If queue is NULL, this function will return NULL.
 *
 * @param queue pointer to queue pointer to get the node from
 * @param position position in the queue to get the node from
 * @param priority of data
 * @return the pointer to the node on success, NULL on failure
 * @note do not free the returned pointer, it is still in the queue
 */
queue_p_node_t *queue_p_get_priority(const queue_p_t *queue, size_t position,
                                     double priority);

/**
 * @brief Get the data from the node at the front of the queue without popping.
 *
 * If queue is NULL, this function will return NULL
 *
 * @param queue pointer to queue pointer to peek
 * @return pointer to peeked queue node on success, NULL on failure
 * @note do not free the returned pointer, it is still in the queue
 */
queue_p_node_t *queue_p_peek(const queue_p_t *queue);

/**
 * @brief Remove the first occurrence of a value in the queue.
 *
 * The returned structure contains information on the dequeued node. The
 * structure must be freed after use.
 *
 * If queue is NULL, this function will return NULL.
 *
 * @param queue pointer to queue to search
 * @param item_to_remove pointer to value to remove
 * @return void* pointer to the removed value, NULL if not found or queue is
 *        NULL
 */
queue_p_node_t *queue_p_remove(queue_p_t *queue, void *item_to_remove);

/**
 * @brief Find the first occurrence of a value in the queue.
 *
 * If queue is NULL, this function will return NULL.
 *
 * @param queue pointer to queue to search
 * @param value_to_find pointer to value to find
 * @return pointer to the first occurrence of the value in the queue, NULL if
 *         not found or queue is NULL
 * @note do not free the returned pointer, it is still in the queue
 */
queue_p_node_t *queue_p_find_first(const queue_p_t *queue,
                                   const void *value_to_find);

/**
 * @brief Clear all nodes out of a queue.
 *
 * If an error occurs, this function will return non-zero.
 * Possible error codes are:
 * - EINVAL: queue is NULL
 *
 * @param queue pointer to queue pointer to clear out
 * @return 0 on success, non-zero value on failure
 */
int queue_p_clear(queue_p_t *queue);

/**
 * @brief Delete a queue.
 *
 * If an error occurs, this function will return non-zero.
 * Possible error codes are:
 * - EINVAL: queue is NULL
 *
 * @param queue pointer to queue pointer to be destroyed
 * @return 0 on success, non-zero value on failure
 */
int queue_p_destroy(queue_p_t **queue);
#endif /* QUEUE_P_H */
