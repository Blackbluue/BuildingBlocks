#include "threadpool_attributes.h"
#include "buildingblocks.h"
#include <errno.h>
#include <stdlib.h>

/* DATA */

#define SUCCESS 0

enum attr_flags {
    DEFAULT_FLAGS = 0,        // no flags
    TIMED_WAIT = 1 << 0,      // true = timed wait, false = infinite wait
    BLOCK_ON_ADD = 1 << 1,    // true = block on add, false = return EAGAIN
    BLOCK_ON_ERR = 1 << 2,    // true = block on error, false = continue running
    THREAD_CREATION = 1 << 3, // true = lazy creation, false = strict creation
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
    if (attr == NULL) {
        DEBUG_PRINT("\tInvalid arguments\n");
        return EINVAL;
    }
    struct inner_threadpool_attr_t *inner =
        ((struct inner_threadpool_attr_t *)attr);
    switch (timed_wait) {
    case TIMED_WAIT_ENABLED:
        DEBUG_PRINT("Enabling timed wait\n");
        inner->flags |= TIMED_WAIT;
        return SUCCESS;
    case TIMED_WAIT_DISABLED:
        DEBUG_PRINT("Disabling timed wait\n");
        inner->flags &= ~TIMED_WAIT;
        return SUCCESS;
    default:
        DEBUG_PRINT("\tInvalid timed wait\n");
        return EINVAL;
    }
}

int threadpool_attr_get_timed_wait(threadpool_attr_t *attr, int *timed_wait) {
    if (attr == NULL || timed_wait == NULL) {
        DEBUG_PRINT("\tInvalid arguments\n");
        return EINVAL;
    }
    struct inner_threadpool_attr_t *inner =
        ((struct inner_threadpool_attr_t *)attr);
    *timed_wait = check_flag(inner->flags, TIMED_WAIT) ? TIMED_WAIT_ENABLED
                                                       : TIMED_WAIT_DISABLED;
    DEBUG_PRINT("Timed wait %s\n", *timed_wait ? "enabled" : "disabled");
    return SUCCESS;
}

int threadpool_attr_set_timeout(threadpool_attr_t *attr, time_t timeout) {
    if (attr == NULL || timeout <= 0) {
        DEBUG_PRINT("\tInvalid arguments\n");
        return EINVAL;
    }
    struct inner_threadpool_attr_t *inner =
        ((struct inner_threadpool_attr_t *)attr);
    inner->default_wait = timeout;
    DEBUG_PRINT("Setting timeout to %ld\n", timeout);
    return SUCCESS;
}

int threadpool_attr_get_timeout(threadpool_attr_t *attr, time_t *timeout) {
    if (attr == NULL || timeout == NULL) {
        DEBUG_PRINT("\tInvalid arguments\n");
        return EINVAL;
    }
    struct inner_threadpool_attr_t *inner =
        ((struct inner_threadpool_attr_t *)attr);
    *timeout = inner->default_wait;
    DEBUG_PRINT("Timeout set to %ld\n", *timeout);
    return SUCCESS;
}

int threadpool_attr_set_block_on_add(threadpool_attr_t *attr,
                                     int block_on_add) {
    if (attr == NULL) {
        DEBUG_PRINT("\tInvalid arguments\n");
        return EINVAL;
    }
    struct inner_threadpool_attr_t *inner =
        ((struct inner_threadpool_attr_t *)attr);
    switch (block_on_add) {
    case BLOCK_ON_ADD_ENABLED:
        DEBUG_PRINT("Enabling block on add\n");
        inner->flags |= BLOCK_ON_ADD;
        return SUCCESS;
    case BLOCK_ON_ADD_DISABLED:
        DEBUG_PRINT("Disabling block on add\n");
        inner->flags &= ~BLOCK_ON_ADD;
        return SUCCESS;
    default:
        DEBUG_PRINT("\tInvalid block on add\n");
        return EINVAL;
    }
}

int threadpool_attr_get_block_on_add(threadpool_attr_t *attr,
                                     int *block_on_add) {
    if (attr == NULL || block_on_add == NULL) {
        DEBUG_PRINT("\tInvalid arguments\n");
        return EINVAL;
    }
    struct inner_threadpool_attr_t *inner =
        ((struct inner_threadpool_attr_t *)attr);
    *block_on_add = check_flag(inner->flags, BLOCK_ON_ADD)
                        ? BLOCK_ON_ADD_ENABLED
                        : BLOCK_ON_ADD_DISABLED;
    DEBUG_PRINT("Block on add %s\n", *block_on_add ? "enabled" : "disabled");
    return SUCCESS;
}

int threadpool_attr_set_block_on_err(threadpool_attr_t *attr,
                                     int block_on_err) {
    if (attr == NULL) {
        DEBUG_PRINT("\tInvalid arguments\n");
        return EINVAL;
    }
    struct inner_threadpool_attr_t *inner =
        ((struct inner_threadpool_attr_t *)attr);
    switch (block_on_err) {
    case BLOCK_ON_ERR_ENABLED:
        DEBUG_PRINT("Enabling block on err\n");
        inner->flags |= BLOCK_ON_ERR;
        return SUCCESS;
    case BLOCK_ON_ERR_DISABLED:
        DEBUG_PRINT("Disabling block on err\n");
        inner->flags &= ~BLOCK_ON_ERR;
        return SUCCESS;
    default:
        DEBUG_PRINT("\tInvalid block on err flag\n");
        return EINVAL;
    }
}

int threadpool_attr_get_block_on_err(threadpool_attr_t *attr,
                                     int *block_on_err) {
    if (attr == NULL || block_on_err == NULL) {
        DEBUG_PRINT("\tInvalid arguments\n");
        return EINVAL;
    }
    struct inner_threadpool_attr_t *inner =
        ((struct inner_threadpool_attr_t *)attr);
    *block_on_err = check_flag(inner->flags, BLOCK_ON_ERR)
                        ? BLOCK_ON_ERR_ENABLED
                        : BLOCK_ON_ERR_DISABLED;
    DEBUG_PRINT("Block on err %s\n", *block_on_err ? "enabled" : "disabled");
    return SUCCESS;
}

int threadpool_attr_set_thread_creation(threadpool_attr_t *attr,
                                        int thread_creation) {
    if (attr == NULL) {
        DEBUG_PRINT("\tInvalid arguments\n");
        return EINVAL;
    }
    struct inner_threadpool_attr_t *inner =
        ((struct inner_threadpool_attr_t *)attr);
    switch (thread_creation) {
    case THREAD_CREATE_LAZY:
        DEBUG_PRINT("Enabling lazy thread creation\n");
        inner->flags |= THREAD_CREATION;
        return SUCCESS;
    case THREAD_CREATE_STRICT:
        DEBUG_PRINT("Enabling strict thread creation\n");
        inner->flags &= ~THREAD_CREATION;
        return SUCCESS;
    default:
        DEBUG_PRINT("\tInvalid thread creation flag\n");
        return EINVAL;
    }
}

int threadpool_attr_get_thread_creation(threadpool_attr_t *attr,
                                        int *thread_creation) {
    if (attr == NULL || thread_creation == NULL) {
        DEBUG_PRINT("\tInvalid arguments\n");
        return EINVAL;
    }
    struct inner_threadpool_attr_t *inner =
        ((struct inner_threadpool_attr_t *)attr);
    *thread_creation = check_flag(inner->flags, THREAD_CREATION)
                           ? THREAD_CREATE_LAZY
                           : THREAD_CREATE_STRICT;
    DEBUG_PRINT("Thread creation set to %s\n",
                *thread_creation ? "lazy" : "strict");
    return SUCCESS;
}

int threadpool_attr_set_thread_count(threadpool_attr_t *attr,
                                     size_t num_threads) {
    if (attr == NULL || num_threads == 0 || num_threads > MAX_THREADS) {
        DEBUG_PRINT("\tInvalid arguments\n");
        return EINVAL;
    }
    struct inner_threadpool_attr_t *inner =
        ((struct inner_threadpool_attr_t *)attr);
    inner->max_threads = num_threads;
    DEBUG_PRINT("Setting thread count to %zu\n", num_threads);
    return SUCCESS;
}

int threadpool_attr_get_thread_count(threadpool_attr_t *attr,
                                     size_t *num_threads) {
    if (attr == NULL || num_threads == NULL) {
        DEBUG_PRINT("\tInvalid arguments\n");
        return EINVAL;
    }
    struct inner_threadpool_attr_t *inner =
        ((struct inner_threadpool_attr_t *)attr);
    *num_threads = inner->max_threads;
    DEBUG_PRINT("Max thread count: %zu\n", *num_threads);
    return SUCCESS;
}

int threadpool_attr_set_queue_size(threadpool_attr_t *attr, size_t queue_size) {
    if (attr == NULL || queue_size == 0) {
        DEBUG_PRINT("\tInvalid arguments\n");
        return EINVAL;
    }
    struct inner_threadpool_attr_t *inner =
        ((struct inner_threadpool_attr_t *)attr);
    inner->max_q_size = queue_size;
    DEBUG_PRINT("Setting queue size to %zu\n", queue_size);
    return SUCCESS;
}

int threadpool_attr_get_queue_size(threadpool_attr_t *attr,
                                   size_t *queue_size) {
    if (attr == NULL || queue_size == NULL) {
        DEBUG_PRINT("\tInvalid arguments\n");
        return EINVAL;
    }
    struct inner_threadpool_attr_t *inner =
        ((struct inner_threadpool_attr_t *)attr);
    *queue_size = inner->max_q_size;
    DEBUG_PRINT("Max queue size: %zu\n", *queue_size);
    return SUCCESS;
}
