#include "queue_p.h"
#include "buildingblocks.h"
#include "linked_list.h"
#include "queue.h"
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>

/* DATA */

#define SUCCESS 0
#define INVALID -1

/**
 * @brief structure of a queue object
 *
 * @param list is the list containing the queue node pointers
 * @param capacity is the number of nodes the queue can hold
 * @param size is the number of nodes the queue is currently storing
 * @param customfree pointer to the user defined free function
 * @compare pointer to the user defined compare function
 */
struct queue_p_t {
    list_t *list;
    size_t capacity;
    size_t size;
    FREE_F customfree;
    CMP_F compare;
};

/* PRIVATE FUNCTIONS */

/**
 * @brief Compare priorities to find the correct queue.
 *
 * Compares a given priority to the priority of a list of queues. Returns 0 if
 * the priority is found, -1 if the priority is greater than the priority of the
 * current queue, and 1 if the priority is less than the priority of the current
 * queue.
 *
 * @param value_to_find pointer to the priority to find
 * @param node_data pointer to the queue to compare
 * @return int the result of the comparison
 */
int q_cmp(const void *value_to_find, const void *node_data) {
    if (value_to_find == NULL || node_data == NULL) {
        return 0; // stable sort on error
    }
    int priority = *(double *)value_to_find;
    queue_p_node_t *node = queue_peek((queue_t *)node_data);
    if (node == NULL) {
        return 0; // stable sort on error
    }
    if (node->priority > priority) {
        return -1;
    } else if (node->priority < priority) {
        return 1;
    } else {
        return 0;
    }
}

/**
 * @brief Free a queue.
 *
 * This function is passed to the list_new function to free the queue when the
 * list is destroyed. It is a wrapper for queue_destroy.
 *
 * @param data pointer to the queue to free
 */
void q_dst(void *data) {
    queue_t *queue = data;
    queue_destroy(&queue);
}

/**
 * @brief Create a new queue and insert it into the list.
 *
 * @param queue pointer to the queue_p_t object
 * @param node pointer to the queue_p_node_t object
 * @param list_idx index to insert the new queue into the list
 * @return int 0 on success, non-zero on failure
 */
int push_new_node(queue_p_t *queue, queue_p_node_t *node, size_t list_idx) {
    if (queue == NULL) {
        return EINVAL;
    }
    int err = SUCCESS;
    queue_t *new_queue =
        queue_init(queue->capacity, free, queue->compare, &err);
    if (new_queue == NULL) {
        free(node);
        return err;
    }
    err = queue_enqueue(new_queue, node);
    if (err != SUCCESS) {
        queue_destroy(&new_queue);
        return err;
    }
    err = list_insert(queue->list, new_queue, list_idx);
    if (err == SUCCESS) {
        queue->size++;
    } else {
        queue_destroy(&new_queue);
    }
    return err;
}

/* PUBLIC FUNCTIONS */

queue_p_t *queue_p_init(size_t capacity, FREE_F customfree, CMP_F compare,
                        int *err) {
    if (compare == NULL) {
        set_err(err, EINVAL);
        return NULL;
    }
    queue_p_t *queue = malloc(sizeof(*queue));
    if (queue == NULL) {
        set_err(err, ENOMEM);
        return NULL;
    }
    queue->list = list_new(q_dst, q_cmp, err);
    if (queue->list == NULL) {
        free(queue);
        return NULL;
    }
    queue->capacity = capacity;
    queue->size = 0;
    queue->customfree = customfree;
    queue->compare = compare;
    return queue;
}

int queue_p_is_full(const queue_p_t *queue) {
    if (queue == NULL) {
        return INVALID;
    } else if (queue->capacity == QUEUE_P_UNLIMITED) {
        return false;
    }
    return queue->size == queue->capacity;
}

int queue_p_is_empty(const queue_p_t *queue) {
    return queue == NULL ? INVALID : queue->size == 0;
}

ssize_t queue_p_capacity(const queue_p_t *queue) {
    return queue == NULL ? INVALID : (ssize_t)queue->capacity;
}

ssize_t queue_p_size(const queue_p_t *queue) {
    return queue == NULL ? INVALID : (ssize_t)queue->size;
}

int queue_p_enqueue(queue_p_t *queue, void *data, double priority) {
    if (queue == NULL) {
        return EINVAL;
    } else if (queue_p_is_full(queue)) {
        return EOVERFLOW;
    }

    queue_p_node_t *node = malloc(sizeof(*node));
    if (node == NULL) {
        return ENOMEM;
    }
    node->data = data;
    node->priority = priority;

    size_t list_idx = 0;
    list_iterator_reset(queue->list);
    queue_t *inner_q = list_iterator_next(queue->list, NULL);
    if (inner_q == NULL) {
        // priority queue is empty, create new queue and insert it into the list
        return push_new_node(queue, node, list_idx);
    }

    do {
        double q_priority = ((queue_p_node_t *)queue_peek(inner_q))->priority;
        if (node->priority < q_priority) {
            // keep looking for the correct queue
            list_idx++;
            continue;
        } else if (node->priority > q_priority) {
            // node priority is greater than current queue priority
            // create new queue and insert it into the list
            return push_new_node(queue, node, list_idx);
        } else {
            // node inserted, stop iterating
            int err = queue_enqueue(inner_q, node);
            if (err == SUCCESS) {
                queue->size++;
            } else {
                free(node);
            }
            return err;
        }
    } while ((inner_q = list_iterator_next(queue->list, NULL)) != NULL);
    // node priority is less than all other queue priorities
    // create new queue and append it to the list
    return push_new_node(queue, node, list_idx);
}

queue_p_node_t *queue_p_dequeue(queue_p_t *queue) {
    if (queue == NULL || queue_p_is_empty(queue)) {
        return NULL;
    }
    queue_t *current_queue = list_peek_head(queue->list);
    queue_p_node_t *node = queue_dequeue(current_queue);
    queue->size--;
    if (queue_is_empty(current_queue)) {
        list_pop_head(queue->list);
        queue_destroy(&current_queue);
    }
    return node;
}

queue_p_node_t *queue_p_get(const queue_p_t *queue, size_t position) {
    if (queue == NULL || position >= queue->size) {
        return NULL;
    }

    list_iterator_reset(queue->list);
    queue_t *inner_q = list_iterator_next(queue->list, NULL);
    do {
        if ((size_t)queue_size(inner_q) > position) {
            return queue_get(inner_q, position);
        } else {
            position -= queue_size(inner_q);
        }
    } while ((inner_q = list_iterator_next(queue->list, NULL)) != NULL);
    // should never reach this point
    return NULL;
}

queue_p_node_t *queue_p_get_priority(const queue_p_t *queue, size_t position,
                                     double priority) {
    if (queue == NULL) {
        return NULL;
    }

    queue_t *current_queue = list_find_first(queue->list, &priority, NULL);
    if (current_queue == NULL) {
        return NULL;
    }
    queue_p_node_t *node = queue_get(current_queue, position);
    if (node == NULL) {
        return NULL;
    }
    return node;
}

queue_p_node_t *queue_p_peek(const queue_p_t *queue) {
    if (queue == NULL || queue_p_is_empty(queue)) {
        return NULL;
    }
    queue_t *current_queue = list_peek_head(queue->list);
    return queue_peek(current_queue);
}

queue_p_node_t *queue_p_remove(queue_p_t *queue, void *item_to_remove) {
    if (queue == NULL || queue_p_is_empty(queue)) {
        return NULL;
    }
    list_iterator_reset(queue->list);
    queue_t *inner_q;
    while ((inner_q = list_iterator_next(queue->list, NULL)) != NULL) {
        queue_p_node_t *node = queue_remove(inner_q, item_to_remove, NULL);
        if (node != NULL) {
            queue->size--;
            if (queue_is_empty(inner_q)) {
                list_remove(queue->list, inner_q, NULL);
                queue_destroy(&inner_q);
            }
            return node;
        }
    }
    return NULL;
}

queue_p_node_t *queue_p_find_first(const queue_p_t *queue,
                                   const void *value_to_find) {
    if (queue == NULL || queue_p_is_empty(queue)) {
        return NULL;
    }
    list_iterator_reset(queue->list);
    queue_t *inner_q;
    while ((inner_q = list_iterator_next(queue->list, NULL)) != NULL) {
        queue_p_node_t *node = queue_find_first(inner_q, value_to_find, NULL);
        if (node != NULL) {
            return node;
        }
    }
    return NULL;
}

int queue_p_clear(queue_p_t *queue) {
    if (queue == NULL) {
        return EINVAL;
    } else if (queue_p_is_empty(queue)) {
        return SUCCESS;
    }
    do {
        queue_t *inner_q = list_pop_head(queue->list);
        do {
            queue_p_node_t *node = queue_dequeue(inner_q);
            if (queue->customfree != NULL) {
                queue->customfree(node->data);
            }
            free(node);
        } while (!queue_is_empty(inner_q));
        queue_destroy(&inner_q);
    } while (!list_is_empty(queue->list));
    queue->size = 0;
    return SUCCESS;
}

int queue_p_destroy(queue_p_t **queue) {
    if (queue == NULL || *queue == NULL) {
        return EINVAL;
    }
    queue_p_clear(*queue);
    list_delete(&(*queue)->list);
    free(*queue);
    *queue = NULL;
    return SUCCESS;
}
