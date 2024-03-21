#include "queue_concurrent.h"
#include "queue.h"
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>

#ifndef __STDC_NO_ATOMICS__
#include <stdatomic.h>
#define ATOMIC_INC(counter, lock) counter++
#define ATOMIC_DEC(counter, lock) counter--
#else
#define ATOMIC_INC(counter, lock)                                              \
    do {                                                                       \
        pthread_mutex_lock(&lock);                                             \
        counter++;                                                             \
        pthread_mutex_unlock(&lock);                                           \
    } while (0);
#define ATOMIC_DEC(counter, lock)                                              \
    do {                                                                       \
        pthread_mutex_lock(&lock);                                             \
        counter--;                                                             \
        pthread_mutex_unlock(&lock);                                           \
    } while (0);
#endif

/* DATA */

#define SUCCESS 0
#define INVALID -1

typedef int (*PREDICATE)(queue_c_t *queue);

/**
 * @brief The deferred signals object.
 *
 * When the queue is manually locked by the user, any signals will be deferred
 * until the user unlocks the queue. This object holds the signals that need to
 * be sent.
 *
 * @param cond_is_empty condition variable to signal the queue is empty
 * @param cond_is_full condition variable to signal the queue is full
 * @param cond_not_empty condition variable to signal the queue is not empty
 * @param cond_not_full condition variable to signal the queue is not full
 * @param is_empty true if the queue is empty
 * @param is_full true if the queue is full
 * @param not_empty true if the queue is not empty
 * @param not_full true if the queue is not full
 */
struct deferred_signals_t {
    pthread_cond_t cond_is_empty;
    pthread_cond_t cond_is_full;
    pthread_cond_t cond_not_empty;
    pthread_cond_t cond_not_full;
    bool is_empty;
    bool is_full;
    bool not_empty;
    bool not_full;
};
/**
 * @brief The concurrent queue object.
 *
 * @param queue pointer to queue object
 * @param lock mutex lock
 * @param counter_lock mutex lock for counter
 * @param signals deferred signals object
 * @param lock_free condition variable to signal the lock is not in use
 * @param waiting_for_lock number of threads waiting for the lock
 * @param waiting_for_cond number of threads waiting for a condition
 * variable
 * @param locked_by thread that manually locked the queue
 * @param manually_locked true if the queue was locked manually by the user
 * @param is_destroying true if the queue is being destroyed
 * @param cancel_wait true if the queue should stop waiting for a condition
 */
struct queue_c_t {
    queue_t *queue;
    pthread_mutex_t lock;
    pthread_mutex_t counter_lock;
    struct deferred_signals_t signals;
    pthread_cond_t lock_free;
#ifndef __STDC_NO_ATOMICS__
    _Atomic size_t waiting_for_lock;
#else
    size_t waiting_for_lock;
#endif
    size_t waiting_for_cond;
    pthread_t locked_by;
    bool manually_locked;
    bool is_destroying;
    bool cancel_wait;
};

/* PRIVATE FUNCTIONS */

/**
 * @brief Initialize thread constructs.
 *
 * @param queue pointer to queue object
 */
static void init_thread_constructs(queue_c_t *queue) {
    if (queue == NULL) {
        return;
    }
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    // to allow wait functions to lock the queue
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK_NP);
    pthread_mutex_init(&queue->lock, &attr);
    pthread_mutexattr_destroy(&attr);

    pthread_mutex_init(&queue->counter_lock, NULL);
    pthread_cond_init(&queue->signals.cond_is_empty, NULL);
    pthread_cond_init(&queue->signals.cond_is_full, NULL);
    pthread_cond_init(&queue->signals.cond_not_empty, NULL);
    pthread_cond_init(&queue->signals.cond_not_full, NULL);
    pthread_cond_init(&queue->lock_free, NULL);

    queue->waiting_for_lock = 0;
    queue->waiting_for_cond = 0;
    queue->signals.is_empty = false;
    queue->signals.is_full = false;
    queue->signals.not_empty = false;
    queue->signals.not_full = false;
    queue->manually_locked = false;
    queue->is_destroying = false;
    queue->cancel_wait = false;
}

/**
 * @brief Destroy thread constructs.
 *
 * @param queue pointer to queue object
 */
static void destroy_thread_constructs(queue_c_t *queue) {
    if (queue == NULL) {
        return;
    }
    // wait for all threads to finish working with the lock
    while (queue->waiting_for_lock > 0) {
        pthread_cond_wait(&queue->lock_free, &queue->lock);
    }
    pthread_mutex_unlock(&queue->lock);

    pthread_mutex_destroy(&queue->lock);
    pthread_mutex_destroy(&queue->counter_lock);
    pthread_cond_destroy(&queue->signals.cond_is_empty);
    pthread_cond_destroy(&queue->signals.cond_is_full);
    pthread_cond_destroy(&queue->signals.cond_not_empty);
    pthread_cond_destroy(&queue->signals.cond_not_full);
    pthread_cond_destroy(&queue->lock_free);
}

/**
 * @brief Wake all threads waiting on queue.
 *
 * @param queue pointer to queue object
 */
static void wake_all_threads(struct deferred_signals_t *signals) {
    if (signals == NULL) {
        return;
    }
    pthread_cond_broadcast(&signals->cond_is_empty);
    pthread_cond_broadcast(&signals->cond_is_full);
    pthread_cond_broadcast(&signals->cond_not_empty);
    pthread_cond_broadcast(&signals->cond_not_full);
}

/**
 * @brief Unlock queue if it was not locked manually by the user.
 *
 * Will also release the manual lock if the queue is being destroyed.
 *
 * @param queue pointer to queue object
 */
static void unlock_queue(queue_c_t *queue) {
    if (queue == NULL) {
        return;
    }
    if (!queue->manually_locked ||
        (queue->manually_locked && queue->is_destroying)) {
        // this thread was the last to decrement the waiting counter. If counter
        // is 0, no other thread is waiting for the lock and we can signal
        // destruction
        if (queue->waiting_for_lock == 0) {
            pthread_cond_signal(&queue->lock_free);
        }
        // can't tell if this thread owns the lock, so we have to try to unlock
        // it and ignore the error
        pthread_mutex_unlock(&queue->lock);
    }
}

/**
 * @brief Lock queue if it was not locked manually by the user.
 *
 * This function maintains a counter of threads waiting to lock the queue.
 *
 * @param queue pointer to queue object
 * @return int 0 if success, error code otherwise
 */
static int lock_queue(queue_c_t *queue) {
    if (queue == NULL || queue->is_destroying) {
        return EINVAL;
    }
    ATOMIC_INC(queue->waiting_for_lock, queue->counter_lock);
    int err = pthread_mutex_lock(&queue->lock);
    ATOMIC_DEC(queue->waiting_for_lock, queue->counter_lock);
    // check if destruction was called while waiting for lock
    if (queue->is_destroying) {
        unlock_queue(queue);
        return EINTR;
    }

    return err;
}

/**
 * @brief Send all signals.
 *
 * If the queue is locked manually by the user, the signals will be deferred
 * until the user unlocks the queue.
 *
 * @param queue pointer to queue object
 */
static void send_signals(queue_c_t *queue) {
    if (queue == NULL) {
        return;
    }
    if (!(queue->manually_locked &&
          pthread_equal(queue->locked_by, pthread_self()))) {
        if (queue->signals.is_empty) {
            pthread_cond_broadcast(&queue->signals.cond_is_empty);
            queue->signals.is_empty = false;
        }
        if (queue->signals.is_full) {
            pthread_cond_broadcast(&queue->signals.cond_is_full);
            queue->signals.is_full = false;
        }
        if (queue->signals.not_empty) {
            pthread_cond_broadcast(&queue->signals.cond_not_empty);
            queue->signals.not_empty = false;
        }
        if (queue->signals.not_full) {
            pthread_cond_broadcast(&queue->signals.cond_not_full);
            queue->signals.not_full = false;
        }
    }
}

/**
 * @brief Manually lock queue.
 *
 * @param queue pointer to queue object
 */
static void manual_lock(queue_c_t *queue) {
    if (queue == NULL) {
        return;
    }
    queue->manually_locked = true;
    queue->locked_by = pthread_self();
}

static int not_full(queue_c_t *queue) { return !queue_c_is_full(queue); }

static int not_empty(queue_c_t *queue) { return !queue_c_is_empty(queue); }

/**
 * @brief Check if the queue should keep waiting.
 *
 * @param queue pointer to queue object
 * @return int 1 if should keep waiting, 0 otherwise
 */
static int keep_waiting(queue_c_t *queue) {
    if (queue == NULL) {
        return 0; // nothing to wait on, so stop waiting
    }
    return !queue->cancel_wait && !queue->is_destroying;
}

/**
 * @brief Wait for condition variable.
 *
 * @param queue pointer to queue object
 * @param condition condition variable to wait for
 * @param pred predicate function
 * @return int 0 if success, error code otherwise
 */
static int wait_for(queue_c_t *queue, pthread_cond_t *cond, PREDICATE pred) {
    if (queue == NULL || queue->is_destroying || cond == NULL || pred == NULL) {
        return EINVAL;
    }
    int err = lock_queue(queue);
    // check for deadlock and queue destruction
    if (err != SUCCESS) {
        return err;
    }
    queue->waiting_for_cond++;
    if ((!pred(queue)) && keep_waiting(queue)) {
        pthread_cond_wait(cond, &queue->lock);
    }
    queue->waiting_for_cond--;
    if (!keep_waiting(queue)) {
        // stop canceling wait if no other thread is waiting for a condition
        if (queue->waiting_for_cond == 0) {
            queue->cancel_wait = false;
        }
        unlock_queue(queue);
        // EINTR takes precedence over EAGAIN
        return queue->is_destroying ? EINTR : EAGAIN;
    }
    manual_lock(queue);
    return SUCCESS;
}

/**
 * @brief Wait for condition variable with timeout.
 *
 * @param queue pointer to queue object
 * @param condition condition variable to wait for
 * @param pred predicate function
 * @param timeout timeout in seconds
 * @return int 0 if success, error code otherwise
 */
static int timed_wait_for(queue_c_t *queue, pthread_cond_t *cond,
                          PREDICATE pred, time_t timeout) {
    if (timeout == 0) {
        return wait_for(queue, cond, pred);
    } else if (queue == NULL || queue->is_destroying || cond == NULL ||
               pred == NULL || timeout < 0) {
        return EINVAL;
    }
    struct timespec abs_timeout = {time(NULL) + timeout, 0};
    int err = lock_queue(queue);
    // check for deadlock and queue destruction
    if (err != SUCCESS) {
        return err;
    }
    queue->waiting_for_cond++;
    if ((!pred(queue)) && keep_waiting(queue)) {
        int err = pthread_cond_timedwait(cond, &queue->lock, &abs_timeout);
        if (err == ETIMEDOUT) {
            queue->waiting_for_cond--;
            unlock_queue(queue);
            return ETIMEDOUT;
        }
    }
    queue->waiting_for_cond--;
    if (!keep_waiting(queue)) {
        // stop canceling wait if no other thread is waiting for a condition
        if (queue->waiting_for_cond == 0) {
            queue->cancel_wait = false;
        }
        unlock_queue(queue);
        // EINTR takes precedence over EAGAIN
        return queue->is_destroying ? EINTR : EAGAIN;
    }
    manual_lock(queue);
    return SUCCESS;
}

/* PUBLIC FUNCTIONS */

queue_c_t *queue_c_init(size_t capacity, FREE_F customfree) {
    queue_c_t *queue_c = malloc(sizeof(*queue_c));
    if (queue_c == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    int err = SUCCESS;
    queue_c->queue = queue_init(capacity, customfree, NULL, &err);
    if (queue_c->queue == NULL) {
        free(queue_c);
        errno = err;
        return NULL;
    }

    init_thread_constructs(queue_c);
    return queue_c;
}

int queue_c_is_full(queue_c_t *queue) {
    if (queue == NULL || queue->is_destroying) {
        return INVALID;
    } else if (queue_capacity(queue->queue) == QUEUE_UNLIMITED) {
        return 0;
    }
    return queue_is_full(queue->queue);
}

int queue_c_wait_for_full(queue_c_t *queue) {
    if (queue == NULL || queue->is_destroying) {
        return EINVAL;
    } else if (queue_capacity(queue->queue) == QUEUE_UNLIMITED) {
        return ENOTSUP;
    }
    return wait_for(queue, &queue->signals.cond_is_full, queue_c_is_full);
}

int queue_c_timed_wait_for_full(queue_c_t *queue, time_t timeout) {
    if (queue == NULL || queue->is_destroying) {
        return EINVAL;
    } else if (queue_capacity(queue->queue) == QUEUE_UNLIMITED) {
        return ENOTSUP;
    }
    return timed_wait_for(queue, &queue->signals.cond_is_full, queue_c_is_full,
                          timeout);
}

int queue_c_wait_for_not_full(queue_c_t *queue) {
    if (queue == NULL || queue->is_destroying) {
        return EINVAL;
    } else if (queue_capacity(queue->queue) == QUEUE_UNLIMITED) {
        return ENOTSUP;
    }
    return wait_for(queue, &queue->signals.cond_not_full, not_full);
}

int queue_c_timed_wait_for_not_full(queue_c_t *queue, time_t timeout) {
    if (queue == NULL || queue->is_destroying) {
        return EINVAL;
    } else if (queue_capacity(queue->queue) == QUEUE_UNLIMITED) {
        return ENOTSUP;
    }
    return timed_wait_for(queue, &queue->signals.cond_not_full, not_full,
                          timeout);
}

int queue_c_is_empty(queue_c_t *queue) {
    if (queue == NULL || queue->is_destroying) {
        return INVALID;
    }
    return queue_is_empty(queue->queue);
}

int queue_c_wait_for_empty(queue_c_t *queue) {
    if (queue == NULL || queue->is_destroying) {
        return EINVAL;
    } else {
        return wait_for(queue, &queue->signals.cond_is_empty, queue_c_is_empty);
    }
}

int queue_c_timed_wait_for_empty(queue_c_t *queue, time_t timeout) {
    if (queue == NULL || queue->is_destroying) {
        return EINVAL;
    } else {
        return timed_wait_for(queue, &queue->signals.cond_is_empty,
                              queue_c_is_empty, timeout);
    }
}

int queue_c_wait_for_not_empty(queue_c_t *queue) {
    if (queue == NULL || queue->is_destroying) {
        return EINVAL;
    } else {
        return wait_for(queue, &queue->signals.cond_not_empty, not_empty);
    }
}

int queue_c_timed_wait_for_not_empty(queue_c_t *queue, time_t timeout) {
    if (queue == NULL || queue->is_destroying) {
        return EINVAL;
    } else {
        return timed_wait_for(queue, &queue->signals.cond_not_empty, not_empty,
                              timeout);
    }
}

int queue_c_cancel_wait(queue_c_t *queue) {
    if (queue == NULL || queue->is_destroying) {
        return EINVAL;
    }
    queue->cancel_wait = true;
    wake_all_threads(&queue->signals);
    return SUCCESS;
}

int queue_c_lock(queue_c_t *queue) {
    if (queue == NULL || queue->is_destroying) {
        return EINVAL;
    }
    int err = lock_queue(queue);
    // check for deadlock and queue destruction
    if (err == SUCCESS) {
        manual_lock(queue);
    }
    return err;
}

int queue_c_unlock(queue_c_t *queue) {
    if (queue == NULL || queue->is_destroying) {
        return EINVAL;
    }
    if (queue->manually_locked &&
        pthread_equal(queue->locked_by, pthread_self())) {
        queue->manually_locked = false;
        send_signals(queue);
        // this thread was the last to decrement the waiting counter. If
        // counter is 0, no other thread is waiting for the lock and we can
        // signal destruction
        if (queue->waiting_for_lock == 0) {
            pthread_cond_signal(&queue->lock_free);
        }
        return pthread_mutex_unlock(&queue->lock);
    }
    return EPERM;
}

ssize_t queue_c_capacity(queue_c_t *queue) {
    if (queue == NULL || queue->is_destroying) {
        return INVALID;
    }
    return queue_capacity(queue->queue);
}

ssize_t queue_c_size(queue_c_t *queue) {
    if (queue == NULL || queue->is_destroying) {
        return INVALID;
    }
    return queue_size(queue->queue);
}

int queue_c_enqueue(queue_c_t *queue, void *data) {
    if (queue == NULL || queue->is_destroying) {
        return EINVAL;
    }

    // deadlock error can be ignored, it was caused by one of the lock functions
    // check if destruction was called while waiting for lock
    if (lock_queue(queue) == EINTR) {
        return EINTR;
    } else if (queue_c_is_full(queue)) {
        unlock_queue(queue);
        return EOVERFLOW;
    }

    int res = queue_enqueue(queue->queue, data);
    if (res != SUCCESS) {
        unlock_queue(queue);
        return res;
    }

    // set up signals
    queue->signals.not_empty = true;
    if (queue_c_is_full(queue)) {
        queue->signals.is_full = true;
    }
    send_signals(queue);
    unlock_queue(queue);
    return SUCCESS;
}

void *queue_c_dequeue(queue_c_t *queue) {
    if (queue == NULL || queue->is_destroying) {
        errno = EINVAL;
        return NULL;
    }

    // deadlock error can be ignored, it was caused by one of the lock functions
    // check if destruction was called while waiting for lock
    if (lock_queue(queue) == EINTR) {
        errno = EINTR;
        return NULL;
    } else if (queue_c_is_empty(queue)) {
        unlock_queue(queue);
        return NULL;
    }

    void *data = queue_dequeue(queue->queue);

    // set up signals
    queue->signals.not_full = true;
    if (queue_c_is_empty(queue)) {
        queue->signals.is_empty = true;
    }
    send_signals(queue);
    unlock_queue(queue);
    return data;
}

void *queue_c_peek(queue_c_t *queue) {
    if (queue == NULL || queue->is_destroying) {
        return NULL;
    }
    return queue_peek(queue->queue);
}

int queue_c_clear(queue_c_t *queue) {
    if (queue == NULL || queue->is_destroying) {
        return EINVAL;
    }
    // deadlock error can be ignored, it was caused by one of the lock functions
    // check if destruction was called while waiting for lock
    if (lock_queue(queue) == EINTR) {
        return EINTR;
    }
    queue_clear(queue->queue);

    // set up signals
    queue->signals.not_full = true;
    queue->signals.is_empty = true;
    send_signals(queue);
    unlock_queue(queue);
    return SUCCESS;
}

int queue_c_destroy(queue_c_t **queue_addr) {
    if (queue_addr == NULL || *queue_addr == NULL ||
        (*queue_addr)->is_destroying) {
        return EINVAL;
    }
    // deadlock error can be ignored, it was caused by one of the lock functions
    // check if destruction was called while waiting for lock
    // pthread_mutex_lock(&(*queue_addr)->lock);
    if (lock_queue((*queue_addr)) == EINTR) {
        return EINTR;
    }

    (*queue_addr)->is_destroying = true;
    wake_all_threads(&(*queue_addr)->signals);

    queue_destroy(&(*queue_addr)->queue);
    destroy_thread_constructs(*queue_addr);
    free(*queue_addr);
    *queue_addr = NULL;
    return SUCCESS;
}
