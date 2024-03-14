#include "queue.h"
#include "linked_list.h"
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>

/*DATA */

#define SUCCESS 0
#define INVALID -1

/**
 * @brief structure of a queue object
 *
 * @param q_data is the list containing the queue node pointers
 * @param capacity is the number of nodes the queue can hold
 * @param support_lookup is a boolean indicating whether the queue supports
 *        lookup
 */
struct queue_t {
    list_t *q_data;
    size_t capacity;
    bool support_lookup;
};

/* PUBLIC FUNCTIONS*/

queue_t *queue_init(size_t capacity, FREE_F customfree, CMP_F compare) {
    queue_t *queue = malloc(sizeof(*queue));
    if (queue == NULL) {
        errno = ENOMEM;
        return NULL;
    }
    queue->q_data = list_new(customfree, compare, NULL);
    if (queue->q_data == NULL) {
        free(queue);
        errno = ENOMEM;
        return NULL;
    }
    queue->support_lookup = compare != NULL;
    queue->capacity = capacity;
    return queue;
}

int queue_is_full(const queue_t *queue) {
    if (queue == NULL) {
        errno = EINVAL;
        return INVALID;
    }
    if (queue->capacity == QUEUE_UNLIMITED) {
        return false;
    }
    return list_size(queue->q_data) == queue->capacity;
}

int queue_is_empty(const queue_t *queue) {
    if (queue == NULL) {
        errno = EINVAL;
        return INVALID;
    }
    return list_size(queue->q_data) == 0;
}

ssize_t queue_capacity(const queue_t *queue) {
    if (queue == NULL) {
        errno = EINVAL;
        return INVALID;
    }
    return queue->capacity;
}

ssize_t queue_size(const queue_t *queue) {
    if (queue == NULL) {
        errno = EINVAL;
        return INVALID;
    }
    return list_size(queue->q_data);
}

int queue_enqueue(queue_t *queue, void *data) {
    if (queue == NULL) {
        return EINVAL;
    } else if (queue_is_full(queue)) {
        return EOVERFLOW;
    }
    return list_push_tail(queue->q_data, data);
}

void *queue_dequeue(queue_t *queue) {
    if (queue == NULL) {
        errno = EINVAL;
        return NULL;
    }
    return list_pop_head(queue->q_data);
}

void *queue_get(const queue_t *queue, size_t position) {
    if (queue == NULL || position >= list_size(queue->q_data)) {
        errno = EINVAL;
        return NULL;
    }
    return list_get(queue->q_data, position);
}

void *queue_peek(const queue_t *queue) {
    if (queue == NULL) {
        errno = EINVAL;
        return NULL;
    }
    return list_peek_head(queue->q_data);
}

void *queue_remove(queue_t *queue, void *item_to_remove) {
    if (queue == NULL) {
        errno = EINVAL;
        return NULL;
    } else if (queue->support_lookup == false) {
        errno = ENOTSUP;
        return NULL;
    }
    return list_remove(queue->q_data, item_to_remove, NULL);
}

void *queue_find_first(const queue_t *queue, const void *value_to_find) {
    if (queue == NULL) {
        errno = EINVAL;
        return NULL;
    } else if (queue->support_lookup == false) {
        errno = ENOTSUP;
        return NULL;
    }
    return list_find_first(queue->q_data, value_to_find, NULL);
}

int queue_clear(queue_t *queue) {
    if (queue == NULL) {
        return EINVAL;
    }
    list_clear(queue->q_data);
    return SUCCESS;
}

int queue_destroy(queue_t **queue_addr) {
    if (queue_addr == NULL || *queue_addr == NULL) {
        return EINVAL;
    }
    list_delete(&(*queue_addr)->q_data);
    free(*queue_addr);
    *queue_addr = NULL;
    return SUCCESS;
}
