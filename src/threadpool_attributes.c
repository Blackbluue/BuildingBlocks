#include "threadpool_attributes.h"
#include "buildingblocks.h"
#include <errno.h>
#include <stdlib.h>

/* DATA */

#define SUCCESS 0

enum attr_flags {
    DEFAULT_FLAGS = 0,     // no flags
    TIMED_WAIT = 1 << 0,   // true = timed wait, false = infinite wait
    BLOCK_ON_ADD = 1 << 1, // true = block on add, false = return EAGAIN
};

struct inner_threadpool_attr_t {
    int flags;          // bit flags
    size_t max_threads; // maximum number of threads
    size_t max_q_size;  // maximum queue size
    // default wait time for blocking calls when timeout not given (in seconds)
    time_t default_wait;
};

/* PRIVATE FUNCTIONS */

/* PUBLIC FUNCTIONS */

int threadpool_attr_init(threadpool_attr_t *attr) {
    if (attr != NULL) {
        struct inner_threadpool_attr_t *inner =
            (struct inner_threadpool_attr_t *)attr;
        inner->flags = DEFAULT_FLAGS;
        inner->max_threads = DEFAULT_THREADS;
        inner->max_q_size = DEFAULT_QUEUE;
        inner->default_wait = DEFAULT_WAIT;
    }
    DEBUG_PRINT("\tAttributes initialized\n");
    return SUCCESS;
}

int threadpool_attr_destroy(threadpool_attr_t *attr) {
    // no need to free memory
    (void)attr;
    DEBUG_PRINT("\tAttributes destroyed\n");
    return SUCCESS;
}

int threadpool_attr_set_timed_wait(threadpool_attr_t *attr, int timed_wait) {
    DEBUG_PRINT("Setting timed wait\n");
    if (attr == NULL) {
        DEBUG_PRINT("\tInvalid arguments\n");
        return EINVAL;
    }
    struct inner_threadpool_attr_t *inner =
        ((struct inner_threadpool_attr_t *)attr);
    switch (timed_wait) {
    case TIMED_WAIT_ENABLED:
        inner->flags |= TIMED_WAIT;
        return SUCCESS;
    case TIMED_WAIT_DISABLED:
        inner->flags &= ~TIMED_WAIT;
        return SUCCESS;
    default:
        DEBUG_PRINT("\tInvalid timed wait\n");
        return EINVAL;
    }
}

int threadpool_attr_get_timed_wait(threadpool_attr_t *attr, int *timed_wait) {
    DEBUG_PRINT("Getting timed wait\n");
    if (attr == NULL || timed_wait == NULL) {
        DEBUG_PRINT("\tInvalid arguments\n");
        return EINVAL;
    }
    struct inner_threadpool_attr_t *inner =
        ((struct inner_threadpool_attr_t *)attr);
    *timed_wait = check_flag(inner->flags, TIMED_WAIT) ? TIMED_WAIT_ENABLED
                                                       : TIMED_WAIT_DISABLED;
    return SUCCESS;
}

int threadpool_attr_set_timeout(threadpool_attr_t *attr, time_t timeout) {
    DEBUG_PRINT("Setting timeout\n");
    if (attr == NULL || timeout <= 0) {
        DEBUG_PRINT("\tInvalid arguments\n");
        return EINVAL;
    }
    struct inner_threadpool_attr_t *inner =
        ((struct inner_threadpool_attr_t *)attr);
    inner->default_wait = timeout;
    return SUCCESS;
}

int threadpool_attr_get_timeout(threadpool_attr_t *attr, time_t *timeout) {
    DEBUG_PRINT("Getting timeout\n");
    if (attr == NULL || timeout == NULL) {
        DEBUG_PRINT("\tInvalid arguments\n");
        return EINVAL;
    }
    struct inner_threadpool_attr_t *inner =
        ((struct inner_threadpool_attr_t *)attr);
    *timeout = inner->default_wait;
    return SUCCESS;
}

int threadpool_attr_set_block_on_add(threadpool_attr_t *attr,
                                     int block_on_add) {
    DEBUG_PRINT("Setting block on add\n");
    if (attr == NULL) {
        DEBUG_PRINT("\tInvalid arguments\n");
        return EINVAL;
    }
    struct inner_threadpool_attr_t *inner =
        ((struct inner_threadpool_attr_t *)attr);
    switch (block_on_add) {
    case BLOCK_ON_ADD_ENABLED:
        inner->flags |= BLOCK_ON_ADD;
        return SUCCESS;
    case BLOCK_ON_ADD_DISABLED:
        inner->flags &= ~BLOCK_ON_ADD;
        return SUCCESS;
    default:
        DEBUG_PRINT("\tInvalid block on add\n");
        return EINVAL;
    }
}

int threadpool_attr_get_block_on_add(threadpool_attr_t *attr,
                                     int *block_on_add) {
    DEBUG_PRINT("Getting block on add\n");
    if (attr == NULL || block_on_add == NULL) {
        DEBUG_PRINT("\tInvalid arguments\n");
        return EINVAL;
    }
    struct inner_threadpool_attr_t *inner =
        ((struct inner_threadpool_attr_t *)attr);
    *block_on_add = check_flag(inner->flags, BLOCK_ON_ADD)
                        ? BLOCK_ON_ADD_ENABLED
                        : BLOCK_ON_ADD_DISABLED;
    return SUCCESS;
}

int threadpool_attr_set_thread_count(threadpool_attr_t *attr,
                                     size_t num_threads) {
    DEBUG_PRINT("Setting thread count\n");
    if (attr == NULL || num_threads == 0 || num_threads > MAX_THREADS) {
        DEBUG_PRINT("\tInvalid arguments\n");
        return EINVAL;
    }
    struct inner_threadpool_attr_t *inner =
        ((struct inner_threadpool_attr_t *)attr);
    inner->max_threads = num_threads;
    return SUCCESS;
}

int threadpool_attr_get_thread_count(threadpool_attr_t *attr,
                                     size_t *num_threads) {
    DEBUG_PRINT("Getting thread count\n");
    if (attr == NULL || num_threads == NULL) {
        DEBUG_PRINT("\tInvalid arguments\n");
        return EINVAL;
    }
    struct inner_threadpool_attr_t *inner =
        ((struct inner_threadpool_attr_t *)attr);
    *num_threads = inner->max_threads;
    return SUCCESS;
}

int threadpool_attr_set_queue_size(threadpool_attr_t *attr, size_t queue_size) {
    DEBUG_PRINT("Setting queue size\n");
    if (attr == NULL || queue_size == 0) {
        DEBUG_PRINT("\tInvalid arguments\n");
        return EINVAL;
    }
    struct inner_threadpool_attr_t *inner =
        ((struct inner_threadpool_attr_t *)attr);
    inner->max_q_size = queue_size;
    return SUCCESS;
}

int threadpool_attr_get_queue_size(threadpool_attr_t *attr,
                                   size_t *queue_size) {
    DEBUG_PRINT("Getting queue size\n");
    if (attr == NULL || queue_size == NULL) {
        DEBUG_PRINT("\tInvalid arguments\n");
        return EINVAL;
    }
    struct inner_threadpool_attr_t *inner =
        ((struct inner_threadpool_attr_t *)attr);
    *queue_size = inner->max_q_size;
    return SUCCESS;
}
