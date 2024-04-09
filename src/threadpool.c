#include "threadpool.h"
#include "debug.h"
#include "queue_concurrent.h"
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>

/* DATA */

#define SUCCESS 0

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
 * @brief Sets the error code.
 *
 * @param err The error code.
 * @param value The value to set.
 */
static void set_err(int *err, int value) {
    if (err != NULL) {
        *err = value;
    }
}

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
    DEBUG_PRINT("\tFreeing threadpool\n");
    free(pool->threads);
    pthread_rwlock_destroy(&pool->running_lock);
    queue_c_destroy(&pool->queue);
    free(pool);
    DEBUG_PRINT("\tThreadpool freed\n");
}

/**
 * @brief Set the default attributes for the threadpool.
 *
 * @param attr pointer to threadpool_attr_t
 */
static void default_attr(threadpool_attr_t *attr) {
    DEBUG_PRINT("\tSetting default attributes\n");
    attr->flags = DEFAULT_FLAGS;
    attr->max_threads = DEFAULT_THREADS;
    attr->max_q_size = DEFAULT_QUEUE;
    attr->default_wait = DEFAULT_WAIT;
    DEBUG_PRINT("\tDefault attributes set\n");
}

/**
 * @brief Initialize the threadpool object.
 *
 * If attr is NULL, the threadpool will be created with the default attributes.
 * Otherwise, the threadpool will be created with the given attributes.
 *
 * Possible errors:
 * - ENOMEM: memory allocation failed
 *
 * @param attr pointer to threadpool_attr_t
 * @param err pointer to error code
 * @return threadpool_t* pointer to threadpool_t, NULL if error
 */
static threadpool_t *init_pool(threadpool_attr_t *attr, int *err) {
    threadpool_t *pool = malloc(sizeof(*pool));
    if (pool == NULL) {
        set_err(err, ENOMEM);
        DEBUG_PRINT("\tFailed to allocate memory for threadpool\n");
        return NULL;
    }

    // use a copy of the supplied attributes so that the user can free them
    // after creating the threadpool
    if (attr == NULL) {
        default_attr(&pool->attr);
    } else {
        DEBUG_PRINT("\tUsing supplied attributes\n");
        pool->attr.flags = attr->flags;
        pool->attr.max_threads = attr->max_threads;
        pool->attr.max_q_size = attr->max_q_size;
        pool->attr.default_wait = attr->default_wait;
    }

    // initialize mutexes and condition variables
    pthread_rwlock_init(&pool->running_lock, NULL);
    pool->num_threads = 0;
    pool->shutdown = NO_SHUTDOWN;
    pool->cancel_type = check_flag(pool->attr.flags, CANCEL_TYPE)
                            ? PTHREAD_CANCEL_ASYNCHRONOUS
                            : PTHREAD_CANCEL_DEFERRED;

    // initialize queue/threads
    pool->queue = queue_c_init(pool->attr.max_q_size, free, NULL);
    if (pool->queue == NULL) {
        DEBUG_PRINT("\tFailed to initialize queue\n");
        goto err;
    }
    pool->threads = malloc(sizeof(*pool->threads) * pool->attr.max_threads);
    if (pool->threads == NULL) {
        DEBUG_PRINT("\tFailed to allocate memory for threads\n");
        goto err;
    }

    DEBUG_PRINT("\tThreadpool initialized\n");
    return pool;
err:
    free_pool(pool);
    set_err(err, ENOMEM);
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
static int add_task(threadpool_t *pool, ROUTINE action, void *arg, void *arg2) {
    struct task_t *task = malloc(sizeof(*task));
    if (task == NULL) {
        queue_c_unlock(pool->queue);
        DEBUG_PRINT("\ton thread %lX: Failed to allocate memory for task\n",
                    pthread_self());
        return ENOMEM;
    }
    task->action = action;
    task->arg = arg;
    task->arg2 = arg2;
    DEBUG_PRINT("\ton thread %lX: \tEnqueueing...\n", pthread_self());
    int res = queue_c_enqueue(pool->queue, task);
    if (res != SUCCESS) {
        queue_c_unlock(pool->queue);
        free(task);
        DEBUG_PRINT("\ton thread %lX: Failed to enqueue task\n",
                    pthread_self());
        return res;
    }
    queue_c_unlock(pool->queue);
    DEBUG_PRINT("\ton thread %lX: Task added to queue\n", pthread_self());
    return SUCCESS;
}

/**
 * @brief Perform a task for the threadpool.
 *
 * @param arg pointer to threadpool_t
 * @return void* NULL
 */
static void *thread_task(void *arg) {
    DEBUG_PRINT("Thread task: thread %lX\n", pthread_self());
    threadpool_t *pool = arg;
    int old_type;
    // determine if the thread can be force cancelled
    pthread_setcanceltype(pool->cancel_type, &old_type);
    DEBUG_PRINT("\ton thread %lX: Thread type set to %d\n", pthread_self(),
                pool->cancel_type);
    for (;;) {
        DEBUG_PRINT("\ton thread %lX: ..Waiting for work\n", pthread_self());
        // wait for work queue to be not empty
        while (queue_c_is_empty(pool->queue) && pool->shutdown == NO_SHUTDOWN) {
            queue_c_wait_for_not_empty(pool->queue);
        }

        // check if threadpool is shutting down
        if (pool->shutdown == SHUTDOWN_FORCEFUL ||
            (pool->shutdown == SHUTDOWN_GRACEFUL &&
             queue_c_is_empty(pool->queue))) {
            DEBUG_PRINT("\ton thread %lX: Thread shutting down\n",
                        pthread_self());
            queue_c_unlock(pool->queue);
            return NULL;
        }

        DEBUG_PRINT("\ton thread %lX: ..Performing work\n", pthread_self());
        // perform work
        struct task_t *task = queue_c_dequeue(pool->queue, NULL);
        DEBUG_PRINT("\ton thread %lX: Work dequeued\n", pthread_self());
        queue_c_unlock(pool->queue);
        ROUTINE action = task->action;
        void *action_arg = task->arg;
        void *action_arg2 = task->arg2;
        free(task);
        pthread_rwlock_rdlock(&pool->running_lock);
        action(action_arg, action_arg2);
        pthread_rwlock_unlock(&pool->running_lock);
        DEBUG_PRINT("\ton thread %lX: Work complete\n", pthread_self());
    }
    return NULL;
}

/* PUBLIC FUNCTIONS */

threadpool_t *threadpool_create(threadpool_attr_t *attr, int *err) {
    DEBUG_PRINT("Creating threadpool\n");
    threadpool_t *pool = init_pool(attr, err);
    if (pool == NULL) {
        return NULL;
    }

    // threadpool creation fails if any thread fails to start
    // TODO: add flag to allow threadpool creation to succeed if some threads
    // fail to start
    for (size_t i = 0; i < pool->attr.max_threads; i++) {
        DEBUG_PRINT("\tCreating thread %zu\n", i);
        int res = pthread_create(&pool->threads[i], NULL, thread_task, pool);
        if (res != SUCCESS) {
            threadpool_destroy(pool, SHUTDOWN_GRACEFUL);
            set_err(err, res);
            DEBUG_PRINT("\tFailed to create thread %zu\n", i);
            return NULL;
        }
        DEBUG_PRINT("\tCreated thread %zu with id: %lX\n", i, pool->threads[i]);
        pool->num_threads++;
    }

    DEBUG_PRINT("\tThreadpool created\n");
    return pool;
}

int threadpool_add_work(threadpool_t *pool, ROUTINE action, void *arg,
                        void *arg2) {
    DEBUG_PRINT("\ton thread %lX: Adding work to threadpool\n", pthread_self());
    if (pool == NULL || action == NULL) {
        DEBUG_PRINT("\ton thread %lX: Invalid arguments\n", pthread_self());
        return EINVAL;
    }

    // timeout ignored if TIMED_WAIT is not set
    if (check_flag(pool->attr.flags, BLOCK_ON_ADD) &&
        check_flag(pool->attr.flags, TIMED_WAIT)) {
        return threadpool_timed_add_work(pool, action, arg, arg2,
                                         pool->attr.default_wait);
    } else if (check_flag(pool->attr.flags, BLOCK_ON_ADD)) {
        DEBUG_PRINT("\ton thread %lX: ...Blocking on add\n", pthread_self());
        while (queue_c_is_full(pool->queue)) {
            queue_c_wait_for_not_full(pool->queue);
        }
    } else {
        queue_c_lock(pool->queue);
        if (queue_c_is_full(pool->queue)) {
            queue_c_unlock(pool->queue);
            DEBUG_PRINT("\ton thread %lX: Queue is full\n", pthread_self());
            return EAGAIN;
        }
    }

    DEBUG_PRINT("\ton thread %lX: Adding task to queue\n", pthread_self());
    return add_task(pool, action, arg, arg2);
}

int threadpool_timed_add_work(threadpool_t *pool, ROUTINE action, void *arg,
                              void *arg2, time_t timeout) {
    DEBUG_PRINT("\ton thread %lX: Adding work to threadpool with timeout\n",
                pthread_self());
    if (pool == NULL || action == NULL || timeout <= 0) {
        DEBUG_PRINT("\ton thread %lX: Invalid arguments\n", pthread_self());
        return EINVAL;
    }

    DEBUG_PRINT("\ton thread %lX: ...Blocking on add with timeout\n",
                pthread_self());
    while (queue_c_is_full(pool->queue)) {
        if (queue_c_timed_wait_for_not_full(pool->queue, timeout) ==
            ETIMEDOUT) {
            DEBUG_PRINT("\ton thread %lX: Timed out\n", pthread_self());
            return ETIMEDOUT;
        }
    }

    DEBUG_PRINT("\ton thread %lX: Adding task to queue\n", pthread_self());
    return add_task(pool, action, arg, arg2);
}

int threadpool_wait(threadpool_t *pool) {
    DEBUG_PRINT("\ton thread %lX: Waiting for threadpool\n", pthread_self());
    if (pool == NULL) {
        DEBUG_PRINT("\ton thread %lX: Invalid arguments\n", pthread_self());
        return EINVAL;
    }
    if (check_flag(pool->attr.flags, TIMED_WAIT)) {
        return threadpool_timed_wait(pool, pool->attr.default_wait);
    }

    DEBUG_PRINT("\ton thread %lX: ...Waiting for queue to be empty\n",
                pthread_self());
    while (!queue_c_is_empty(pool->queue)) {
        queue_c_wait_for_empty(pool->queue);
    }

    // cant acquire the write lock until all readers are done
    pthread_rwlock_wrlock(&pool->running_lock);

    // all threads are idle
    pthread_rwlock_unlock(&pool->running_lock);
    queue_c_unlock(pool->queue);
    DEBUG_PRINT("\ton thread %lX: All tasks complete\n", pthread_self());
    return SUCCESS;
}

int threadpool_timed_wait(threadpool_t *pool, time_t timeout) {
    DEBUG_PRINT("\ton thread %lX: Waiting for threadpool with timeout\n",
                pthread_self());
    if (pool == NULL || timeout <= 0) {
        DEBUG_PRINT("\ton thread %lX: Invalid arguments\n", pthread_self());
        return EINVAL;
    }

    struct timespec abstime = {time(NULL) + timeout, 0};

    DEBUG_PRINT(
        "\ton thread %lX: ...Waiting for queue to be empty with timeout\n",
        pthread_self());
    while (!queue_c_is_empty(pool->queue)) {
        if (queue_c_timed_wait_for_empty(pool->queue, timeout) == ETIMEDOUT) {
            DEBUG_PRINT("\ton thread %lX: Timed out\n", pthread_self());
            return ETIMEDOUT;
        }
    }

    // cant acquire the write lock until all readers are done
    int res = pthread_rwlock_timedwrlock(&pool->running_lock, &abstime);
    if (res == ETIMEDOUT) {
        queue_c_unlock(pool->queue);
        DEBUG_PRINT("\ton thread %lX: Timed out\n", pthread_self());
        return ETIMEDOUT;
    }

    // all threads are idle
    pthread_rwlock_unlock(&pool->running_lock);
    queue_c_unlock(pool->queue);
    DEBUG_PRINT("\ton thread %lX: All tasks complete\n", pthread_self());
    return SUCCESS;
}

int threadpool_destroy(threadpool_t *pool, int flag) {
    DEBUG_PRINT("\ton thread %lX: Destroying threadpool\n", pthread_self());
    if (pool == NULL ||
        (flag != SHUTDOWN_GRACEFUL && flag != SHUTDOWN_FORCEFUL)) {
        DEBUG_PRINT("\ton thread %lX: Invalid arguments\n", pthread_self());
        return EINVAL;
    }
    if (flag == SHUTDOWN_GRACEFUL) {
        threadpool_wait(pool);
    }
    // wake up all threads
    pool->shutdown = flag;
    DEBUG_PRINT("\ton thread %lX: Waking threads\n", pthread_self());
    queue_c_cancel_wait(pool->queue);
    for (size_t i = 0; i < pool->num_threads; i++) {
        if (flag == SHUTDOWN_FORCEFUL) {
            // will be ignored if thread is already cancelled
            DEBUG_PRINT("\ton thread %lX: Cancelling thread %zu with id %lX\n",
                        i, pool->threads[i], pthread_self());
            pthread_cancel(pool->threads[i]);
        }
        DEBUG_PRINT("\ton thread %lX: Joining thread %zu with id %lX\n", i,
                    pool->threads[i], pthread_self());
        pthread_join(pool->threads[i], NULL);
        DEBUG_PRINT("\ton thread %lX: Thread %zu joined\n", i, pthread_self());
    }
    free_pool(pool);
    DEBUG_PRINT("\ton thread %lX: Threadpool destroyed\n", pthread_self());
    return SUCCESS;
}

threadpool_attr_t *threadpool_attr_init(void) {
    DEBUG_PRINT("Initializing threadpool attributes\n");
    threadpool_attr_t *attr = malloc(sizeof(*attr));
    if (attr == NULL) {
        DEBUG_PRINT("\tFailed to allocate memory for attributes\n");
        errno = ENOMEM;
        return NULL;
    }
    default_attr(attr);
    DEBUG_PRINT("\tAttributes initialized\n");
    return attr;
}

int threadpool_attr_destroy(threadpool_attr_t *attr) {
    DEBUG_PRINT("Destroying threadpool attributes\n");
    if (attr == NULL) {
        DEBUG_PRINT("\tInvalid arguments\n");
        return EINVAL;
    }
    free(attr);
    DEBUG_PRINT("\tAttributes destroyed\n");
    return SUCCESS;
}

int threadpool_attr_set_cancel_type(threadpool_attr_t *attr, int cancel_type) {
    DEBUG_PRINT("Setting cancel type\n");
    if (attr == NULL) {
        DEBUG_PRINT("\tInvalid arguments\n");
        return EINVAL;
    }
    switch (cancel_type) {
    case CANCEL_ASYNC:
        attr->flags |= CANCEL_TYPE;
        DEBUG_PRINT("\tCancel type set to asynchronous\n");
        return SUCCESS;
    case CANCEL_DEFERRED:
        attr->flags &= ~CANCEL_TYPE;
        DEBUG_PRINT("\tCancel type set to deferred\n");
        return SUCCESS;
    default:
        DEBUG_PRINT("\tInvalid cancel type\n");
        return EINVAL;
    }
}

int threadpool_attr_get_cancel_type(threadpool_attr_t *attr, int *cancel_type) {
    DEBUG_PRINT("Getting cancel type\n");
    if (attr == NULL || cancel_type == NULL) {
        DEBUG_PRINT("\tInvalid arguments\n");
        return EINVAL;
    }
    *cancel_type =
        check_flag(attr->flags, CANCEL_TYPE) ? CANCEL_ASYNC : CANCEL_DEFERRED;
    DEBUG_PRINT("\tCancel type: %d\n", *cancel_type);
    return SUCCESS;
}

int threadpool_attr_set_timed_wait(threadpool_attr_t *attr, int timed_wait) {
    DEBUG_PRINT("Setting timed wait\n");
    if (attr == NULL) {
        DEBUG_PRINT("\tInvalid arguments\n");
        return EINVAL;
    }
    switch (timed_wait) {
    case TIMED_WAIT_ENABLED:
        attr->flags |= TIMED_WAIT;
        DEBUG_PRINT("\tTimed wait enabled\n");
        return SUCCESS;
    case TIMED_WAIT_DISABLED:
        attr->flags &= ~TIMED_WAIT;
        DEBUG_PRINT("\tTimed wait disabled\n");
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
    *timed_wait = check_flag(attr->flags, TIMED_WAIT) ? TIMED_WAIT_ENABLED
                                                      : TIMED_WAIT_DISABLED;
    DEBUG_PRINT("\tTimed wait: %d\n", *timed_wait);
    return SUCCESS;
}

int threadpool_attr_set_timeout(threadpool_attr_t *attr, time_t timeout) {
    DEBUG_PRINT("Setting timeout\n");
    if (attr == NULL || timeout <= 0) {
        DEBUG_PRINT("\tInvalid arguments\n");
        return EINVAL;
    }
    attr->default_wait = timeout;
    DEBUG_PRINT("\tTimeout set to %ld\n", timeout);
    return SUCCESS;
}

int threadpool_attr_get_timeout(threadpool_attr_t *attr, time_t *timeout) {
    DEBUG_PRINT("Getting timeout\n");
    if (attr == NULL || timeout == NULL) {
        DEBUG_PRINT("\tInvalid arguments\n");
        return EINVAL;
    }
    *timeout = attr->default_wait;
    DEBUG_PRINT("\tTimeout: %ld\n", *timeout);
    return SUCCESS;
}

int threadpool_attr_set_block_on_add(threadpool_attr_t *attr,
                                     int block_on_add) {
    DEBUG_PRINT("Setting block on add\n");
    if (attr == NULL) {
        DEBUG_PRINT("\tInvalid arguments\n");
        return EINVAL;
    }
    switch (block_on_add) {
    case BLOCK_ON_ADD_ENABLED:
        attr->flags |= BLOCK_ON_ADD;
        DEBUG_PRINT("\tBlocking on add enabled\n");
        return SUCCESS;
    case BLOCK_ON_ADD_DISABLED:
        attr->flags &= ~BLOCK_ON_ADD;
        DEBUG_PRINT("\tBlocking on add disabled\n");
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
    *block_on_add = check_flag(attr->flags, BLOCK_ON_ADD)
                        ? BLOCK_ON_ADD_ENABLED
                        : BLOCK_ON_ADD_DISABLED;
    DEBUG_PRINT("\tBlock on add: %d\n", *block_on_add);
    return SUCCESS;
}

int threadpool_attr_set_thread_count(threadpool_attr_t *attr,
                                     size_t num_threads) {
    DEBUG_PRINT("Setting thread count\n");
    if (attr == NULL || num_threads == 0 || num_threads > MAX_THREADS) {
        DEBUG_PRINT("\tInvalid arguments\n");
        return EINVAL;
    }
    attr->max_threads = num_threads;
    DEBUG_PRINT("\tThread count set to %zu\n", num_threads);
    return SUCCESS;
}

int threadpool_attr_get_thread_count(threadpool_attr_t *attr,
                                     size_t *num_threads) {
    DEBUG_PRINT("Getting thread count\n");
    if (attr == NULL || num_threads == NULL) {
        DEBUG_PRINT("\tInvalid arguments\n");
        return EINVAL;
    }
    *num_threads = attr->max_threads;
    DEBUG_PRINT("\tThread count: %zu\n", *num_threads);
    return SUCCESS;
}

int threadpool_attr_set_queue_size(threadpool_attr_t *attr, size_t queue_size) {
    DEBUG_PRINT("Setting queue size\n");
    if (attr == NULL || queue_size == 0) {
        DEBUG_PRINT("\tInvalid arguments\n");
        return EINVAL;
    }
    attr->max_q_size = queue_size;
    DEBUG_PRINT("\tQueue size set to %zu\n", queue_size);
    return SUCCESS;
}

int threadpool_attr_get_queue_size(threadpool_attr_t *attr,
                                   size_t *queue_size) {
    DEBUG_PRINT("Getting queue size\n");
    if (attr == NULL || queue_size == NULL) {
        DEBUG_PRINT("\tInvalid arguments\n");
        return EINVAL;
    }
    *queue_size = attr->max_q_size;
    DEBUG_PRINT("\tQueue size: %zu\n", *queue_size);
    return SUCCESS;
}
