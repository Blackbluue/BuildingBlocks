#define _DEFAULT_SOURCE
#include "threadpool.h"
#include "buildingblocks.h"
#include "queue_concurrent.h"
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/* DATA */

#define SUCCESS 0
#define LOCK_WAIT_TIMEOUT 3

// casting for pthread start routine
typedef void *(*THRD)(void *);

struct task_t {
    ROUTINE action;
    void *arg;
};

struct thread {
    size_t index;
    pthread_t id;
    threadpool_t *pool;
    struct task_t *task;
    pthread_mutex_t info_lock;
    thread_status status;
    int error;
    pthread_cond_t error_cond;
    task_type type;
    pthread_cond_t type_cond;
};

struct threadpool_t {
    struct thread *threads;
    struct thread_info *info;
    pthread_rwlock_t running_lock;
    pthread_mutex_t pool_lock;
    queue_c_t *queue;
    size_t num_threads;
    size_t max_threads;
    bool lock_requested;
    size_t locked_thread;
    pthread_cond_t lock_cond;
    int shutdown;
    int timed_wait;
    int block_on_add;
    int block_on_err;
    int thread_creation;
    time_t default_wait;
};

/* PRIVATE FUNCTIONS */

/**
 * @brief Set the status of the thread.
 *
 * @param self pointer to thread object
 * @param status status to set
 */
static void set_status(struct thread *self, thread_status status) {
    pthread_mutex_lock(&self->info_lock);
    self->status = status;
    pthread_mutex_unlock(&self->info_lock);
}

/**
 * @brief Deallocate memory for the threadpool object
 *
 * @param pool pointer to threadpool_t
 */
static void free_pool(threadpool_t *pool) {
    DEBUG_PRINT("\tFreeing threadpool\n");
    for (size_t i = 0; i < pool->max_threads; i++) {
        struct thread *thread = &pool->threads[i];
        pthread_cond_destroy(&thread->error_cond);
        pthread_cond_destroy(&thread->type_cond);
        pthread_mutex_destroy(&thread->info_lock);
        if (thread->task != NULL) {
            free(thread->task);
        }
    }
    free(pool->threads);
    free(pool->info);
    pthread_rwlock_destroy(&pool->running_lock);
    pthread_mutex_destroy(&pool->pool_lock);
    pthread_cond_destroy(&pool->lock_cond);
    queue_c_destroy(&pool->queue);
    free(pool);
}

/**
 * @brief Initialize the threadpool object with thread information.
 *
 * @param pool pointer to threadpool_t
 * @param err pointer to error code
 * @return threadpool_t* pointer to threadpool_t, NULL if error
 */
static threadpool_t *init_thread_info(threadpool_t *pool, int *err) {
    if (pool == NULL) {
        set_err(err, EINVAL);
        return NULL;
    }
    pool->threads = malloc(sizeof(*pool->threads) * pool->max_threads);
    pool->info = malloc(sizeof(*pool->info) * pool->max_threads);
    if (pool->threads == NULL || pool->info == NULL) {
        DEBUG_PRINT("\tFailed to allocate memory for threads\n");
        pool->max_threads = 0; // prevent attempted freeing of thread details
        free_pool(pool);
        set_err(err, ENOMEM);
        return NULL;
    }
    for (size_t i = 0; i < pool->max_threads; i++) {
        DEBUG_PRINT("\tInitializing thread info %zu\n", i);
        struct thread *thread = &pool->threads[i];
        thread->index = i;
        thread->pool = pool;
        thread->task = NULL;
        pthread_mutex_init(&thread->info_lock, NULL);
        thread->status = STOPPED;
        thread->error = SUCCESS;
        pthread_cond_init(&thread->error_cond, NULL);
        thread->type = UNSPECIFIED;
        pthread_cond_init(&thread->type_cond, NULL);

        struct thread_info *info = &pool->info[i];
        info->index = i;
        info->action = NULL;
        info->arg = NULL;
        info->status = STOPPED;
        info->type = UNSPECIFIED;
        info->error = SUCCESS;
    }
    DEBUG_PRINT("\tThreadpool initialized\n");
    return pool;
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
        DEBUG_PRINT("\tUsing default attributes\n");
        threadpool_attr_t temp_attr;
        threadpool_attr_init(&temp_attr);
        attr = &temp_attr;
    }
    size_t q_size;
    threadpool_attr_get_queue_size(attr, &q_size);
    threadpool_attr_get_thread_count(attr, &pool->max_threads);
    threadpool_attr_get_block_on_add(attr, &pool->block_on_add);
    threadpool_attr_get_block_on_err(attr, &pool->block_on_err);
    threadpool_attr_get_thread_creation(attr, &pool->thread_creation);
    threadpool_attr_get_timed_wait(attr, &pool->timed_wait);
    threadpool_attr_get_timeout(attr, &pool->default_wait);

    // initialize mutexes and condition variables
    pthread_rwlock_init(&pool->running_lock, NULL);
    pthread_mutex_init(&pool->pool_lock, NULL);
    pthread_cond_init(&pool->lock_cond, NULL);
    pool->lock_requested = false;
    pool->locked_thread = 0;
    pool->num_threads = 0;
    pool->shutdown = NO_SHUTDOWN;

    // initialize queue/threads
    pool->queue = queue_c_init(q_size, free, err);
    if (pool->queue == NULL) {
        DEBUG_PRINT("\tFailed to initialize queue\n");
        pool->max_threads = 0; // prevent attempted freeing of thread details
        free_pool(pool);
        return NULL;
    }
    return init_thread_info(pool, err);
}

/**
 * @brief Wait for error to be cleared.
 *
 * If block_on_err is enabled, the thread will wait for any error to be cleared.
 * If block_on_err is disabled, the thread will return immediately.
 *
 * @param self pointer to thread
 */
static void wait_on_error(struct thread *self) {
    if (self->pool->block_on_err == BLOCK_ON_ERR_ENABLED) {
        while (self->error != SUCCESS) {
            self->status = BLOCKED;
            pthread_cond_wait(&self->error_cond, &self->info_lock);
        }
    }
}

/**
 * @brief Perform a task for the threadpool.
 *
 * @param self pointer to thread
 */
static void thread_task(struct thread *self) {
    DEBUG_PRINT("Starting worker %lX\n", pthread_self());
    threadpool_t *pool = self->pool;
    set_status(self, IDLE);
    for (;;) {
        DEBUG_PRINT("\ton thread %lX: ..Waiting for work\n", pthread_self());
        // wait for work queue to be not empty
        while (queue_c_is_empty(pool->queue) && pool->shutdown == NO_SHUTDOWN) {
            int err = queue_c_wait_for_not_empty(pool->queue);
            if (!(err == SUCCESS || err == EAGAIN)) {
                // EAGAIN returned if queue wait is manually canceled
                DEBUG_PRINT("\ton thread %lX: Error waiting for work\n",
                            pthread_self());
                queue_c_unlock(pool->queue);
                set_status(self, STOPPED);
                return;
            } else if (err == EAGAIN) {
                // check if thread lock requested
                pthread_mutex_lock(&pool->pool_lock);
                if (pool->lock_requested) {
                    pthread_mutex_lock(&self->info_lock);
                    self->status = LOCKED;
                    self->type = UNSPECIFIED;
                    pthread_mutex_unlock(&self->info_lock);
                    pool->lock_requested = false;
                    pool->locked_thread = self->index;
                    pthread_cond_signal(&pool->lock_cond);
                    pthread_mutex_unlock(&pool->pool_lock);
                    return;
                }
                pthread_mutex_unlock(&pool->pool_lock);
            }
        }

        // check if threadpool is shutting down
        if (pool->shutdown == SHUTDOWN_FORCEFUL ||
            (pool->shutdown == SHUTDOWN_GRACEFUL &&
             queue_c_is_empty(pool->queue))) {
            DEBUG_PRINT("\ton thread %lX: Thread shutting down\n",
                        pthread_self());
            queue_c_unlock(pool->queue);
            set_status(self, DESTROYING);
            return;
        }

        DEBUG_PRINT("\ton thread %lX: ..Performing work\n", pthread_self());
        // perform work
        pthread_mutex_lock(&self->info_lock);
        self->task = queue_c_dequeue(pool->queue, NULL);
        queue_c_unlock(pool->queue);
        if (self->task == NULL) {
            DEBUG_PRINT("\ton thread %lX: Failed to dequeue task\n",
                        pthread_self());
            pthread_mutex_unlock(&self->info_lock);
            continue;
        }
        self->status = RUNNING;
        pthread_mutex_unlock(&self->info_lock);
        DEBUG_PRINT("\ton thread %lX: Work dequeued\n", pthread_self());
        pthread_rwlock_rdlock(&pool->running_lock);
        int err = self->task->action(self->task->arg);
        pthread_rwlock_unlock(&pool->running_lock);
        pthread_mutex_lock(&self->info_lock);
        self->error = err;
        wait_on_error(self);
        free(self->task);
        self->task = NULL;
        self->status = IDLE;
        pthread_mutex_unlock(&self->info_lock);
        DEBUG_PRINT("\ton thread %lX: Work complete\n", pthread_self());
    }
}

/**
 * @brief Perform a dedicated task for the threadpool.
 *
 * @param self pointer to thread
 */
static void dedicated_task(struct thread *self) {
    DEBUG_PRINT("Starting dedicated %lX\n", pthread_self());
    set_status(self, RUNNING);

    DEBUG_PRINT("\ton thread %lX: ..Performing work\n", pthread_self());
    // perform work
    int err = self->task->action(self->task->arg);
    pthread_mutex_lock(&self->info_lock);
    self->error = err;
    free(self->task);
    self->task = NULL;
    self->type = UNSPECIFIED;
    DEBUG_PRINT("\ton thread %lX: Work complete\n", pthread_self());
    // check if threadpool is shutting down
    if (self->pool->shutdown != NO_SHUTDOWN) {
        DEBUG_PRINT("\ton thread %lX: Thread shutting down\n", pthread_self());
        self->status = DESTROYING;
    } else {
        // reset status to locked. application can check on the error then
        self->status = LOCKED;
    }
    pthread_mutex_unlock(&self->info_lock);
}

/**
 * @brief Task coordinator for the thread.
 *
 * The thread will wait for a task type to be assigned to it. Either worker or
 * dedicated.
 *
 * @param self pointer to thread
 * @return void* NULL
 */
static void *task_coordinator(struct thread *self) {
    DEBUG_PRINT("Starting thread %lX\n", pthread_self());

    threadpool_t *pool = self->pool;
    for (;;) {
        pthread_mutex_lock(&self->info_lock);
        while (self->type == UNSPECIFIED && pool->shutdown == NO_SHUTDOWN) {
            pthread_cond_wait(&self->type_cond, &self->info_lock);
        }
        // check if threadpool is shutting down
        if (pool->shutdown != NO_SHUTDOWN) {
            self->status = DESTROYING;
            pthread_mutex_unlock(&self->info_lock);
            DEBUG_PRINT("Destroying thread %lX\n", pthread_self());
            return NULL;
        }

        task_type type = self->type;
        pthread_mutex_unlock(&self->info_lock);
        switch (type) {
        case WORKER:
            // perform work from queue
            thread_task(self);
            break;
        case DEDICATED:
            // perform dedicated task
            dedicated_task(self);
            break;
        default: // error in type, exit thread
            return NULL;
        }
        DEBUG_PRINT("Thread %lX: Checking for destruction\n", pthread_self());
        pthread_mutex_lock(&self->info_lock);
        if (self->status == DESTROYING) {
            pthread_mutex_unlock(&self->info_lock);
            DEBUG_PRINT("Destroying thread %lX\n", pthread_self());
            return NULL;
        }
        pthread_mutex_unlock(&self->info_lock);
    }
}

/**
 * @brief Lock an idle thread.
 *
 * @param pool pointer to threadpool_t
 * @param thread_idx pointer to thread index
 * @retval 0 if successful
 * @retval EAGAIN if no threads available
 */
static int lock_idle(threadpool_t *pool, size_t *thread_idx) {
    int locked = EAGAIN;
    if (pool != NULL) {
        pthread_mutex_lock(&pool->pool_lock);
        pool->lock_requested = true;
        // wake all threads, ask 1 to lock
        queue_c_cancel_wait(pool->queue);
        struct timespec abstime = {time(NULL) + LOCK_WAIT_TIMEOUT, 0};
        // locked thread will set lock_requested to false
        while (pool->lock_requested == true) {
            pthread_cond_timedwait(&pool->lock_cond, &pool->pool_lock,
                                   &abstime);
        }
        // if lock requested is false, thread is locked
        if (pool->lock_requested == false) {
            if (thread_idx != NULL) {
                *thread_idx = pool->locked_thread;
            }
            locked = SUCCESS;
        }
        pthread_mutex_unlock(&pool->pool_lock);
    }
    return locked;
}

/**
 * @brief Lock a stopped thread.
 *
 * @param pool pointer to threadpool_t
 * @param thread_idx pointer to thread index
 * @retval 0 if successful
 * @retval EAGAIN if no threads available
 */
static int lock_stopped(threadpool_t *pool, size_t *thread_idx) {
    int locked = EAGAIN;
    if (pool != NULL) {
        for (size_t i = 0; i < pool->max_threads; i++) {
            struct thread *thread = &pool->threads[i];
            pthread_mutex_lock(&thread->info_lock);
            if (thread->status == STOPPED) {
                thread->status = LOCKED;
                int res = pthread_create(&thread->id, NULL,
                                         (THRD)task_coordinator, thread);
                if (res == SUCCESS) {
                    DEBUG_PRINT("\tLocking thread %lX\n", thread->id);
                    if (thread_idx != NULL) {
                        *thread_idx = i;
                    }
                    pthread_mutex_unlock(&thread->info_lock);
                    locked = SUCCESS;
                    break;
                } else {
                    DEBUG_PRINT("\tFailed to start/lock thread %zu\n", i);
                    thread->status = STOPPED;
                    thread->type = UNSPECIFIED;
                }
            }
            pthread_mutex_unlock(&thread->info_lock);
        }
    }
    return locked;
}

/**
 * @brief Start a new thread.
 *
 * If the threadpool is not at capacity, a new thread will be started.
 *
 * @param pool pointer to threadpool_t
 * @return int 0 if successful, otherwise error code
 */
static int start_new_thread(threadpool_t *pool) {
    if (pool->num_threads >= pool->max_threads) {
        return SUCCESS;
    }
    // look for the first available thread
    int res;
    for (size_t i = 0; i < pool->max_threads; i++) {
        struct thread *thread = &pool->threads[i];
        pthread_mutex_lock(&thread->info_lock);
        switch (thread->status) {
        case STARTING:
            if (thread->type == UNSPECIFIED) {
                // only signal the thread to start if it is not already starting
                DEBUG_PRINT("\tWake thread %zu with id %lX\n", i, thread->id);
                thread->type = WORKER;
                pthread_cond_signal(&thread->type_cond);
                return SUCCESS;
            }
            // keep looking
            break;
        case STOPPED:
            thread->status = STARTING;
            thread->type = WORKER;
            res = pthread_create(&thread->id, NULL, (THRD)task_coordinator,
                                 thread);
            if (res == SUCCESS) {
                DEBUG_PRINT("\tStart thread %zu with id %lX\n", i, thread->id);
                pool->num_threads++;
            } else {
                thread->status = STOPPED;
                thread->type = UNSPECIFIED;
            }
            pthread_mutex_unlock(&thread->info_lock);
            return res;
        case IDLE:
            // assume this thread will eventually pick up the task
            DEBUG_PRINT("\tThread %zu is idle, let it take the task\n", i);
            pthread_mutex_unlock(&thread->info_lock);
            return SUCCESS;
        default: // keep looking
            break;
        }
        pthread_mutex_unlock(&thread->info_lock);
    }
    return EAGAIN;
}

/**
 * @brief Add a task to the queue.
 *
 * @param pool pointer to threadpool_t
 * @param action pointer to function to be performed
 * @param arg pointer to argument for action
 * @return int 0 if successful, otherwise error code
 */
static int add_task(threadpool_t *pool, ROUTINE action, void *arg) {
    struct task_t *task = malloc(sizeof(*task));
    if (task == NULL) {
        queue_c_unlock(pool->queue);
        DEBUG_PRINT("\ton thread %lX: Failed to allocate memory for task\n",
                    pthread_self());
        return ENOMEM;
    }
    task->action = action;
    task->arg = arg;
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
    // lazy creation allows for threads to be created on demand
    // even if thread creation fails, the task still stays in the queue
    if (pool->thread_creation == THREAD_CREATE_LAZY) {
        res = start_new_thread(pool);
        if (res != SUCCESS) {
            DEBUG_PRINT("\ton thread %lX: Failed to start new thread '%s'\n",
                        pthread_self(), strerror(res));
            return res;
        }
    }
    return SUCCESS;
}

/**
 * @brief Restart a thread.
 *
 * @param thread pointer to thread
 * @return int 0 if successful, otherwise error code
 */
static int restart_thread(struct thread *thread) {
    if (thread == NULL) {
        return EINVAL;
    }
    int res = EALREADY;
    pthread_mutex_lock(&thread->info_lock);
    switch (thread->status) {
    case BLOCKED: // thread is blocked
        thread->error = SUCCESS;
        pthread_cond_signal(&thread->error_cond);
        res = SUCCESS;
        break;
    case STOPPED: // thread is not running
        thread->status = STARTING;
        thread->type = WORKER;
        res = pthread_create(&thread->id, NULL, (THRD)task_coordinator, thread);
        if (res == SUCCESS) {
            DEBUG_PRINT("\tRestarted thread %lX\n", thread->id);
        } else {
            thread->status = STOPPED;
            thread->type = UNSPECIFIED;
        }
        break;
    default:
        break;
    }
    pthread_mutex_unlock(&thread->info_lock);
    return res;
}

/* PUBLIC FUNCTIONS */

threadpool_t *threadpool_create(threadpool_attr_t *attr, int *err) {
    DEBUG_PRINT("Creating threadpool\n");
    threadpool_t *pool = init_pool(attr, err);
    if (pool == NULL) {
        return NULL;
    }

    // strict creation requires all threads to be created before returning
    if (pool->thread_creation == THREAD_CREATE_STRICT) {
        for (size_t i = 0; i < pool->max_threads; i++) {
            struct thread *thread = &pool->threads[i];
            thread->status = STARTING;
            int res = pthread_create(&thread->id, NULL, (THRD)task_coordinator,
                                     thread);
            if (res != SUCCESS) {
                threadpool_destroy(pool, SHUTDOWN_GRACEFUL);
                set_err(err, res);
                DEBUG_PRINT("\tFailed to create thread %zu\n", i);
                return NULL;
            }
            DEBUG_PRINT("\tCreated thread %zu with id: %lX\n", i, thread->id);
            pool->num_threads++;
        }
    }
    DEBUG_PRINT("\tThreadpool created\n");
    return pool;
}

int threadpool_add_work(threadpool_t *pool, ROUTINE action, void *arg) {
    DEBUG_PRINT("\ton thread %lX: Adding work to threadpool\n", pthread_self());
    if (pool == NULL || action == NULL) {
        DEBUG_PRINT("\ton thread %lX: Invalid arguments\n", pthread_self());
        return EINVAL;
    }

    // timeout ignored if TIMED_WAIT is not set
    if (pool->block_on_add == BLOCK_ON_ADD_ENABLED &&
        pool->timed_wait == TIMED_WAIT_ENABLED) {
        return threadpool_timed_add_work(pool, action, arg, pool->default_wait);
    } else if (pool->block_on_add) {
        DEBUG_PRINT("\ton thread %lX: ...Blocking on add\n", pthread_self());
        while (queue_c_is_full(pool->queue)) {
            queue_c_wait_for_not_full(pool->queue);
        }
    } else {
        queue_c_lock(pool->queue);
        if (queue_c_is_full(pool->queue)) {
            queue_c_unlock(pool->queue);
            DEBUG_PRINT("\ton thread %lX: Queue is full\n", pthread_self());
            return EOVERFLOW;
        }
    }

    DEBUG_PRINT("\ton thread %lX: Adding task to queue\n", pthread_self());
    return add_task(pool, action, arg);
}

int threadpool_timed_add_work(threadpool_t *pool, ROUTINE action, void *arg,
                              time_t timeout) {
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
    return add_task(pool, action, arg);
}

int threadpool_lock_thread(threadpool_t *pool, size_t *thread_idx) {
    if (pool == NULL) {
        DEBUG_PRINT("\ton thread %lX: pool is null\n", pthread_self());
        return EINVAL;
    }
    int locked = EAGAIN;
    // if strict, most threads should be idle
    // if lazy, most threads should be stopped
    if (pool->thread_creation == THREAD_CREATE_STRICT) {
        // prioritize idle threads
        locked = lock_idle(pool, thread_idx);
        if (locked == EAGAIN) {
            // look for stopped threads if none are idle
            locked = lock_stopped(pool, thread_idx);
        }
    } else {
        // prioritize stopped threads
        locked = lock_stopped(pool, thread_idx);
        if (locked == EAGAIN) {
            // look for idle threads if none are stopped
            locked = lock_idle(pool, thread_idx);
        }
    }
    DEBUG_PRINT("\ton thread %lX: Locking %s\n", pthread_self(),
                locked == SUCCESS ? "successful" : "failed");
    return locked;
}

int threadpool_unlock_thread(threadpool_t *pool, size_t thread_idx) {
    if (pool == NULL) {
        DEBUG_PRINT("\ton thread %lX: pool is null\n", pthread_self());
        return EINVAL;
    } else if (thread_idx >= pool->max_threads) {
        DEBUG_PRINT("\ton thread %lX: %zu is not a valid thread\n",
                    pthread_self(), thread_idx);
        return ENOENT;
    }
    struct thread *thread = &pool->threads[thread_idx];
    pthread_mutex_lock(&thread->info_lock);
    if (thread->status == LOCKED) {
        thread->status = STARTING;
        DEBUG_PRINT("\ton thread %lX: Unlocked thread %zu\n", pthread_self(),
                    thread_idx);
    }
    pthread_mutex_unlock(&thread->info_lock);
    return SUCCESS;
}

int threadpool_add_dedicated(threadpool_t *pool, ROUTINE action, void *arg,
                             size_t thread_idx) {
    if (pool == NULL || action == NULL) {
        DEBUG_PRINT("\ton thread %lX: Invalid arguments\n", pthread_self());
        return EINVAL;
    } else if (thread_idx >= pool->max_threads) {
        DEBUG_PRINT("\ton thread %lX: %zu is not a valid thread\n",
                    pthread_self(), thread_idx);
        return ENOENT;
    }

    int res = EAGAIN;
    struct thread *thread = &pool->threads[thread_idx];
    pthread_mutex_lock(&thread->info_lock);
    if (thread->status == LOCKED) {
        thread->task = malloc(sizeof(*thread->task));
        if (thread->task == NULL) {
            res = ENOMEM;
            goto cleanup;
        }
        thread->task->action = action;
        thread->task->arg = arg;
        thread->type = DEDICATED;
        pthread_cond_signal(&thread->type_cond);
        res = SUCCESS;
    }

cleanup:
    DEBUG_PRINT("\ton thread %lX: Dedicated thread %zu is %s started\n",
                pthread_self(), thread_idx,
                res == SUCCESS ? "successfully" : "unsuccessfully");
    pthread_mutex_unlock(&thread->info_lock);
    return res;
}

int threadpool_thread_status(threadpool_t *pool, size_t thread_idx,
                             struct thread_info *info) {
    DEBUG_PRINT("\ton thread %lX: Getting thread status\n", pthread_self());
    if (pool == NULL || info == NULL) {
        DEBUG_PRINT("\ton thread %lX: Invalid arguments\n", pthread_self());
        return EINVAL;
    } else if (thread_idx >= pool->max_threads) {
        DEBUG_PRINT("\ton thread %lX: %zu is not a valid thread\n",
                    pthread_self(), thread_idx);
        return ENOENT;
    }
    struct thread *thread = &pool->threads[thread_idx];
    pthread_mutex_lock(&thread->info_lock);
    info->index = thread_idx;
    info->action = thread->task->action;
    info->arg = thread->task->arg;
    info->status = thread->status;
    info->type = thread->type;
    info->error = thread->error;
    pthread_mutex_unlock(&thread->info_lock);
    DEBUG_PRINT("\ton thread %lX: Thread %zu status: %d\n", pthread_self(),
                thread_idx, info->status);
    return SUCCESS;
}

int threadpool_thread_status_all(threadpool_t *pool,
                                 struct thread_info **info_arr) {
    DEBUG_PRINT("\ton thread %lX: Getting all thread statuses\n",
                pthread_self());
    if (pool == NULL) {
        DEBUG_PRINT("\ton thread %lX: Invalid arguments\n", pthread_self());
        return EINVAL;
    }

    for (size_t i = 0; i < pool->max_threads; i++) {
        threadpool_thread_status(pool, i, &pool->info[i]);
    }
    if (info_arr != NULL) {
        *info_arr = pool->info;
    }
    DEBUG_PRINT("\ton thread %lX: All thread statuses retrieved\n",
                pthread_self());
    return SUCCESS;
}

int threadpool_restart_thread(threadpool_t *pool, size_t thread_idx) {
    DEBUG_PRINT("\ton thread %lX: Restarting thread %zu\n", pthread_self(),
                thread_idx);
    if (pool == NULL) {
        DEBUG_PRINT("\ton thread %lX: Invalid arguments\n", pthread_self());
        return EINVAL;
    } else if (thread_idx >= pool->max_threads) {
        DEBUG_PRINT("\ton thread %lX: %zu is not a valid thread\n",
                    pthread_self(), thread_idx);
        return ENOENT;
    }

    return restart_thread(&pool->threads[thread_idx]);
}

int threadpool_refresh(threadpool_t *pool) {
    DEBUG_PRINT("\ton thread %lX: Refreshing all threads\n", pthread_self());
    if (pool == NULL) {
        DEBUG_PRINT("\ton thread %lX: Invalid arguments\n", pthread_self());
        return EINVAL;
    }

    for (size_t i = 0; i < pool->max_threads; i++) {
        struct thread *thread = &pool->threads[i];
        if (pool->thread_creation == THREAD_CREATE_LAZY &&
            thread->status != BLOCKED) {
            // only refresh blocked threads when using lazy creation
            continue;
        }
        if (restart_thread(thread) != SUCCESS) {
            DEBUG_PRINT("\ton thread %lX: Failed to refresh thread %zu\n",
                        pthread_self(), i);
            return EAGAIN;
        }
    }
    return SUCCESS;
}

int threadpool_wait(threadpool_t *pool) {
    if (pool == NULL) {
        DEBUG_PRINT("\ton thread %lX: Invalid arguments\n", pthread_self());
        return EINVAL;
    }
    if (pool->timed_wait == TIMED_WAIT_ENABLED) {
        return threadpool_timed_wait(pool, pool->default_wait);
    }

    DEBUG_PRINT("\ton thread %lX: ...Waiting for threadpool\n", pthread_self());
    // this loop is skipped if the queue is empty upon first check,
    // meaning the queue is never locked. bug?
    while (!queue_c_is_empty(pool->queue)) {
        int res = queue_c_wait_for_empty(pool->queue);
        if (res != SUCCESS) {
            DEBUG_PRINT("\ton thread %lX: stop waiting on queue: %s\n",
                        pthread_self(), strerror(res));
            return res;
        }
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
        int res = queue_c_timed_wait_for_empty(pool->queue, timeout);
        if (res != SUCCESS) {
            DEBUG_PRINT("\ton thread %lX: stop waiting on queue: %s\n",
                        pthread_self(), strerror(res));
            return res;
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

int threadpool_cancel_wait(threadpool_t *pool) {
    if (pool == NULL) {
        DEBUG_PRINT("\ton thread %lX: Invalid arguments\n", pthread_self());
        return EINVAL;
    }
    DEBUG_PRINT("\ton thread %lX: Cancelling wait on threadpool\n",
                pthread_self());
    queue_c_cancel_wait(pool->queue);
    return SUCCESS;
}

int threadpool_signal_all(threadpool_t *pool, int sig) {
    if (pool == NULL) {
        DEBUG_PRINT("\ton thread %lX: Invalid arguments\n", pthread_self());
        return EINVAL;
    } else if (sigaction(sig, NULL, NULL) == EINVAL) {
        DEBUG_PRINT("\ton thread %lX: Invalid signal\n", pthread_self());
        return EINVAL;
    }

    DEBUG_PRINT("\ton thread %lX: Signaling threadpool: '%s'\n", pthread_self(),
                strsignal(sig));
    for (size_t i = 0; i < pool->max_threads; i++) {
        struct thread *thread = &pool->threads[i];
        pthread_mutex_lock(&thread->info_lock);
        if (thread->status == RUNNING) {
            pthread_kill(thread->id, sig);
        }
        pthread_mutex_unlock(&thread->info_lock);
    }
    return SUCCESS;
}

int threadpool_signal(threadpool_t *pool, size_t thread_idx, int sig) {
    if (pool == NULL) {
        DEBUG_PRINT("\ton thread %lX: Invalid arguments\n", pthread_self());
        return EINVAL;
    } else if (thread_idx >= pool->max_threads) {
        DEBUG_PRINT("\ton thread %lX: %zu is not a valid thread\n",
                    pthread_self(), thread_idx);
        return ENOENT;
    } else if (sigaction(sig, NULL, NULL) == EINVAL) {
        DEBUG_PRINT("\ton thread %lX: Invalid signal\n", pthread_self());
        return EINVAL;
    }

    DEBUG_PRINT("\ton thread %lX: Signaling thread %zu: '%s'\n", pthread_self(),
                thread_idx, strsignal(sig));
    struct thread *thread = &pool->threads[thread_idx];
    pthread_mutex_lock(&thread->info_lock);
    if (thread->status == RUNNING) {
        pthread_kill(thread->id, sig);
    }
    pthread_mutex_unlock(&thread->info_lock);
    return SUCCESS;
}

int threadpool_destroy(threadpool_t *pool, int flag) {
    if (pool == NULL ||
        (flag != SHUTDOWN_GRACEFUL && flag != SHUTDOWN_FORCEFUL)) {
        DEBUG_PRINT("\ton thread %lX: Invalid arguments\n", pthread_self());
        return EINVAL;
    }
    DEBUG_PRINT("\ton thread %lX: Destroying threadpool\n", pthread_self());
    if (flag == SHUTDOWN_GRACEFUL) {
        threadpool_wait(pool);
    }
    // wake up all threads, then allow them to end on their own
    pool->shutdown = flag;
    queue_c_cancel_wait(pool->queue);
    for (size_t i = 0; i < pool->max_threads; i++) {
        struct thread *thread = &pool->threads[i];
        pthread_mutex_lock(&thread->info_lock);
        switch (thread->status) {
        case STOPPED:
            // skip threads that are not running
            continue;
        case STARTING:
        case LOCKED:
            // wake up starting/locked threads
            DEBUG_PRINT("\ton thread %lX: Unblocking thread %zu with id %lX\n",
                        pthread_self(), i, thread->id);
            pthread_cond_signal(&thread->type_cond);
            break;
        default:
            break;
        }
        pthread_mutex_unlock(&thread->info_lock);

        if (flag == SHUTDOWN_FORCEFUL) {
            // will be ignored if thread is already cancelled
            DEBUG_PRINT("\ton thread %lX: Cancelling thread %zu with id %lX\n",
                        pthread_self(), i, thread->id);
            pthread_cancel(thread->id);
        }
        DEBUG_PRINT("\ton thread %lX: Joining thread %zu with id %lX\n",
                    pthread_self(), i, thread->id);
        pthread_join(thread->id, NULL);
        DEBUG_PRINT("\ton thread %lX: Thread %zu joined\n", pthread_self(), i);
    }
    free_pool(pool);
    DEBUG_PRINT("\ton thread %lX: Threadpool destroyed\n", pthread_self());
    return SUCCESS;
}
