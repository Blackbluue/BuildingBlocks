#include "threadpool.h"
#include "queue_concurrent.h"
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>

/* DATA */

enum {
    SUCCESS = 0,
};
enum attr_flags {
    DEFAULT_FLAGS = 0,     // no flags
    CANCEL_TYPE = 1 << 0,  // true = asynchronous, false = deferred
    TIMED_WAIT = 1 << 1,   // true = timed wait, false = infinite wait
    BLOCK_ON_ADD = 1 << 2, // true = block on add, false = return EAGAIN
};

struct task_t {
    ROUTINE action;
    void *arg;
    void *arg2;
};

struct threadpool_attr_t {
    int flags;
    size_t max_threads;
    size_t max_q_size;
    time_t default_wait;
};

struct threadpool_t {
    pthread_t *threads;
    pthread_rwlock_t running_lock;
    queue_c_t *queue;
    size_t num_threads;
    int shutdown;
    int cancel_type;
    threadpool_attr_t attr;
};

/* PRIVATE FUNCTIONS */

/**
 * @brief Check if a flag is set.
 *
 * @param flags The flag to check.
 * @param to_check The flag to check against.
 * @return non-zero if the flag is set.
 * @return 0 if the flag is not set.
 */
static inline int check_flag(int flags, int to_check) {
    return (flags & to_check) == to_check;
}

/**
 * @brief Deallocate memory for the threadpool object
 *
 * @param pool pointer to threadpool_t
 */
static void free_pool(threadpool_t *pool) {
    free(pool->threads);
    pthread_rwlock_destroy(&pool->running_lock);
    queue_c_destroy(&pool->queue);
    free(pool);
}

/**
 * @brief Set the default attributes for the threadpool.
 *
 * @param attr pointer to threadpool_attr_t
 */
static void default_attr(threadpool_attr_t *attr) {
    attr->flags = DEFAULT_FLAGS;
    attr->max_threads = DEFAULT_THREADS;
    attr->max_q_size = DEFAULT_QUEUE;
    attr->default_wait = DEFAULT_WAIT;
}

/**
 * @brief Initialize the threadpool object.
 *
 * If attr is NULL, the threadpool will be created with the default attributes.
 * Otherwise, the threadpool will be created with the given attributes. If an
 * error occurs while allocating memory for the threadpool, NULL will be
 * returned.
 *
 *
 * @param attr pointer to threadpool_attr_t
 * @return threadpool_t* pointer to threadpool_t
 */
static threadpool_t *init_pool(threadpool_attr_t *attr) {
    threadpool_t *pool = malloc(sizeof(*pool));
    if (pool == NULL) {
        return NULL;
    }

    // use a copy of the supplied attributes so that the user can free them
    // after creating the threadpool
    if (attr == NULL) {
        default_attr(&pool->attr);
    } else {
        pool->attr.flags = attr->flags;
        pool->attr.max_threads = attr->max_threads;
        pool->attr.max_q_size = attr->max_q_size;
        pool->attr.default_wait = attr->default_wait;
    }

    // initialize mutexes and condition variables
    pthread_rwlock_init(&pool->running_lock, NULL);
    pool->num_threads = 0;
    pool->shutdown = 0;
    pool->cancel_type = check_flag(pool->attr.flags, CANCEL_TYPE)
                            ? PTHREAD_CANCEL_ASYNCHRONOUS
                            : PTHREAD_CANCEL_DEFERRED;

    // initialize queue/threads
    pool->queue = queue_c_init(pool->attr.max_q_size, free);
    if (pool->queue == NULL) {
        goto err;
    }
    pool->threads = malloc(sizeof(*pool->threads) * pool->attr.max_threads);
    if (pool->threads == NULL) {
        goto err;
    }

    return pool;
err:
    free_pool(pool);
    errno = ENOMEM;
    return NULL;
}

/**
 * @brief Add a task to the queue.
 *
 * @param pool pointer to threadpool_t
 * @param action pointer to function to be performed
 * @param arg pointer to argument for action
 * @param arg2 pointer to second argument for action
 * @return int 0 if successful, otherwise error code
 */
int add_task(threadpool_t *pool, ROUTINE action, void *arg, void *arg2) {
    struct task_t *task = malloc(sizeof(*task));
    if (task == NULL) {
        queue_c_unlock(pool->queue);
        return ENOMEM;
    }
    task->action = action;
    task->arg = arg;
    task->arg2 = arg2;
    int res = queue_c_enqueue(pool->queue, task);
    if (res != SUCCESS) {
        queue_c_unlock(pool->queue);
        free(task);
        return res;
    }
    queue_c_unlock(pool->queue);
    return SUCCESS;
}

/**
 * @brief Perform a task for the threadpool.
 *
 * @param arg pointer to threadpool_t
 * @return void* NULL
 */
static void *thread_task(void *arg) {
    threadpool_t *pool = arg;
    int old_type;
    // determine if the thread can be force cancelled
    pthread_setcanceltype(pool->cancel_type, &old_type);
    for (;;) {
        // wait for work queue to be not empty
        while (queue_c_is_empty(pool->queue) && pool->shutdown == 0) {
            queue_c_wait_for_not_empty(pool->queue);
        }

        // check if threadpool is shutting down
        if (pool->shutdown == SHUTDOWN_FORCEFUL ||
            (pool->shutdown == SHUTDOWN_GRACEFUL &&
             queue_c_is_empty(pool->queue))) {
            queue_c_unlock(pool->queue);
            return NULL;
        }

        // perform work
        struct task_t *task = queue_c_dequeue(pool->queue);
        queue_c_unlock(pool->queue);
        ROUTINE action = task->action;
        void *action_arg = task->arg;
        void *action_arg2 = task->arg2;
        free(task);
        pthread_rwlock_rdlock(&pool->running_lock);
        action(action_arg, action_arg2);
        pthread_rwlock_unlock(&pool->running_lock);
    }
    return NULL;
}

/* PUBLIC FUNCTIONS */

threadpool_t *threadpool_create(threadpool_attr_t *attr) {
    threadpool_t *pool = init_pool(attr);
    if (pool == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    // threadpool creation fails if any thread fails to start
    // TODO: add flag to allow threadpool creation to succeed if some threads
    // fail to start
    for (size_t i = 0; i < pool->attr.max_threads; i++) {
        int res = pthread_create(&pool->threads[i], NULL, thread_task, pool);
        if (res != SUCCESS) {
            threadpool_destroy(pool, SHUTDOWN_GRACEFUL);
            errno = res;
            return NULL;
        }
        pool->num_threads++;
    }

    return pool;
}

int threadpool_add_work(threadpool_t *pool, ROUTINE action, void *arg,
                        void *arg2) {
    if (pool == NULL || action == NULL) {
        return EINVAL;
    }

    // timeout ignored if TIMED_WAIT is not set
    if (check_flag(pool->attr.flags, BLOCK_ON_ADD) &&
        check_flag(pool->attr.flags, TIMED_WAIT)) {
        return threadpool_timed_add_work(pool, action, arg, arg2,
                                         pool->attr.default_wait);
    } else if (check_flag(pool->attr.flags, BLOCK_ON_ADD)) {
        while (queue_c_is_full(pool->queue)) {
            queue_c_wait_for_not_full(pool->queue);
        }
    } else {
        queue_c_lock(pool->queue);
        if (queue_c_is_full(pool->queue)) {
            queue_c_unlock(pool->queue);
            return EAGAIN;
        }
    }

    return add_task(pool, action, arg, arg2);
}

int threadpool_timed_add_work(threadpool_t *pool, ROUTINE action, void *arg,
                              void *arg2, time_t timeout) {
    if (pool == NULL || action == NULL || timeout <= 0) {
        return EINVAL;
    }

    while (queue_c_is_full(pool->queue)) {
        if (queue_c_timed_wait_for_not_full(pool->queue, timeout) ==
            ETIMEDOUT) {
            return ETIMEDOUT;
        }
    }

    return add_task(pool, action, arg, arg2);
}

int threadpool_wait(threadpool_t *pool) {
    if (pool == NULL) {
        return EINVAL;
    }
    if (check_flag(pool->attr.flags, TIMED_WAIT)) {
        return threadpool_timed_wait(pool, pool->attr.default_wait);
    }

    while (!queue_c_is_empty(pool->queue)) {
        queue_c_wait_for_empty(pool->queue);
    }

    // cant acquire the write lock until all readers are done
    pthread_rwlock_wrlock(&pool->running_lock);

    // all threads are idle
    pthread_rwlock_unlock(&pool->running_lock);
    queue_c_unlock(pool->queue);
    return SUCCESS;
}

int threadpool_timed_wait(threadpool_t *pool, time_t timeout) {
    if (pool == NULL || timeout <= 0) {
        return EINVAL;
    }

    struct timespec abstime = {time(NULL) + timeout, 0};

    while (!queue_c_is_empty(pool->queue)) {
        if (queue_c_timed_wait_for_empty(pool->queue, timeout) == ETIMEDOUT) {
            return ETIMEDOUT;
        }
    }

    // cant acquire the write lock until all readers are done
    int res = pthread_rwlock_timedwrlock(&pool->running_lock, &abstime);
    if (res == ETIMEDOUT) {
        queue_c_unlock(pool->queue);
        return ETIMEDOUT;
    }

    // all threads are idle
    pthread_rwlock_unlock(&pool->running_lock);
    queue_c_unlock(pool->queue);
    return SUCCESS;
}

int threadpool_destroy(threadpool_t *pool, int flag) {
    if (pool == NULL ||
        (flag != SHUTDOWN_GRACEFUL && flag != SHUTDOWN_FORCEFUL)) {
        return EINVAL;
    }
    if (flag == SHUTDOWN_GRACEFUL) {
        threadpool_wait(pool);
    }
    // wake up all threads
    pool->shutdown = flag;
    queue_c_cancel_wait(pool->queue);
    for (size_t i = 0; i < pool->num_threads; i++) {
        if (flag == SHUTDOWN_FORCEFUL) {
            // will be ignored if thread is already cancelled
            pthread_cancel(pool->threads[i]);
        }
        pthread_join(pool->threads[i], NULL);
    }
    free_pool(pool);
    return SUCCESS;
}

threadpool_attr_t *threadpool_attr_init(void) {
    threadpool_attr_t *attr = malloc(sizeof(*attr));
    if (attr == NULL) {
        errno = ENOMEM;
        return NULL;
    }
    default_attr(attr);
    return attr;
}

int threadpool_attr_destroy(threadpool_attr_t *attr) {
    if (attr == NULL) {
        return EINVAL;
    }
    free(attr);
    return SUCCESS;
}

int threadpool_attr_set_cancel_type(threadpool_attr_t *attr, int cancel_type) {
    if (attr == NULL) {
        return EINVAL;
    }
    switch (cancel_type) {
    case CANCEL_ASYNC:
        attr->flags |= CANCEL_TYPE;
        return SUCCESS;
    case CANCEL_DEFERRED:
        attr->flags &= ~CANCEL_TYPE;
        return SUCCESS;
    default:
        return EINVAL;
    }
}

int threadpool_attr_get_cancel_type(threadpool_attr_t *attr, int *cancel_type) {
    if (attr == NULL || cancel_type == NULL) {
        return EINVAL;
    }
    *cancel_type =
        check_flag(attr->flags, CANCEL_TYPE) ? CANCEL_ASYNC : CANCEL_DEFERRED;
    return SUCCESS;
}

int threadpool_attr_set_timed_wait(threadpool_attr_t *attr, int timed_wait) {
    if (attr == NULL) {
        return EINVAL;
    }
    switch (timed_wait) {
    case TIMED_WAIT_ENABLED:
        attr->flags |= TIMED_WAIT;
        return SUCCESS;
    case TIMED_WAIT_DISABLED:
        attr->flags &= ~TIMED_WAIT;
        return SUCCESS;
    default:
        return EINVAL;
    }
}

int threadpool_attr_get_timed_wait(threadpool_attr_t *attr, int *timed_wait) {
    if (attr == NULL || timed_wait == NULL) {
        return EINVAL;
    }
    *timed_wait = check_flag(attr->flags, TIMED_WAIT) ? TIMED_WAIT_ENABLED
                                                      : TIMED_WAIT_DISABLED;
    return SUCCESS;
}

int threadpool_attr_set_timeout(threadpool_attr_t *attr, time_t timeout) {
    if (attr == NULL || timeout <= 0) {
        return EINVAL;
    }
    attr->default_wait = timeout;
    return SUCCESS;
}

int threadpool_attr_get_timeout(threadpool_attr_t *attr, time_t *timeout) {
    if (attr == NULL || timeout == NULL) {
        return EINVAL;
    }
    *timeout = attr->default_wait;
    return SUCCESS;
}

int threadpool_attr_set_block_on_add(threadpool_attr_t *attr,
                                     int block_on_add) {
    if (attr == NULL) {
        return EINVAL;
    }
    switch (block_on_add) {
    case BLOCK_ON_ADD_ENABLED:
        attr->flags |= BLOCK_ON_ADD;
        return SUCCESS;
    case BLOCK_ON_ADD_DISABLED:
        attr->flags &= ~BLOCK_ON_ADD;
        return SUCCESS;
    default:
        return EINVAL;
    }
}

int threadpool_attr_get_block_on_add(threadpool_attr_t *attr,
                                     int *block_on_add) {
    if (attr == NULL || block_on_add == NULL) {
        return EINVAL;
    }
    *block_on_add = check_flag(attr->flags, BLOCK_ON_ADD)
                        ? BLOCK_ON_ADD_ENABLED
                        : BLOCK_ON_ADD_DISABLED;
    return SUCCESS;
}

int threadpool_attr_set_thread_count(threadpool_attr_t *attr,
                                     size_t num_threads) {
    if (attr == NULL || num_threads == 0 || num_threads > MAX_THREADS) {
        return EINVAL;
    }
    attr->max_threads = num_threads;
    return SUCCESS;
}

int threadpool_attr_get_thread_count(threadpool_attr_t *attr,
                                     size_t *num_threads) {
    if (attr == NULL || num_threads == NULL) {
        return EINVAL;
    }
    *num_threads = attr->max_threads;
    return SUCCESS;
}

int threadpool_attr_set_queue_size(threadpool_attr_t *attr, size_t queue_size) {
    if (attr == NULL || queue_size == 0) {
        return EINVAL;
    }
    attr->max_q_size = queue_size;
    return SUCCESS;
}

int threadpool_attr_get_queue_size(threadpool_attr_t *attr,
                                   size_t *queue_size) {
    if (attr == NULL || queue_size == NULL) {
        return EINVAL;
    }
    *queue_size = attr->max_q_size;
    return SUCCESS;
}
