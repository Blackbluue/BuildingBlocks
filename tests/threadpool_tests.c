#include "threadpool.h"
#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>

enum {
    THREAD_COUNT = 4,
    QUEUE_SIZE = 10,
    TIMEOUT = 1,
    SUCCESS = 0,
    GOOD_FLAG = 0,
    BAD_FLAG = -1,
    GOOD_COUNT = 2,
};
pthread_mutex_t lock;

typedef int (*attr_flag_set)(threadpool_attr_t *attr, int flag);
typedef int (*attr_flag_get)(threadpool_attr_t *attr, int *flag);
typedef int (*attr_count_set)(threadpool_attr_t *attr, size_t count);
typedef int (*attr_count_get)(threadpool_attr_t *attr, size_t *count);
typedef int (*attr_timeout_set)(threadpool_attr_t *attr, time_t timeout);
typedef int (*attr_timeout_get)(threadpool_attr_t *attr, time_t *timeout);
#define ATTR_SET(name) threadpool_attr_set_##name
#define ATTR_GET(name) threadpool_attr_get_##name
int init_suite1(void) {
    if (pthread_mutex_init(&lock, NULL) != SUCCESS) {
        return -1;
    }
    return 0;
}

int clean_suite1(void) {
    if (pthread_mutex_destroy(&lock) != SUCCESS) {
        return -1;
    }
    return 0;
}

void attr_flag_test_fail(attr_flag_set set, attr_flag_get get,
                         threadpool_attr_t *attr) {
    CU_ASSERT_EQUAL(set(attr, BAD_FLAG), EINVAL);
    CU_ASSERT_EQUAL(set(NULL, GOOD_FLAG), EINVAL);

    int result_flag;
    CU_ASSERT_EQUAL(get(NULL, &result_flag), EINVAL);
    CU_ASSERT_EQUAL(get(attr, NULL), EINVAL);
}
#define ATTR_FLAG_TEST_FAIL(name, attr)                                        \
    attr_flag_test_fail(ATTR_SET(name), ATTR_GET(name), attr)

void attr_flag_test_pass(attr_flag_set set, attr_flag_get get,
                         threadpool_attr_t *attr, int value) {
    int result_flag = BAD_FLAG;
    CU_ASSERT_EQUAL(set(attr, value), SUCCESS);
    CU_ASSERT_EQUAL(get(attr, &result_flag), SUCCESS);
    CU_ASSERT_EQUAL(result_flag, value);
}
#define ATTR_FLAG_TEST_PASS(name, attr, value)                                 \
    attr_flag_test_pass(ATTR_SET(name), ATTR_GET(name), attr, value)

void attr_count_test_fail(attr_count_set set, attr_count_get get,
                          threadpool_attr_t *attr, size_t bad_count) {
    CU_ASSERT_EQUAL(set(attr, bad_count), EINVAL);
    CU_ASSERT_EQUAL(set(NULL, GOOD_COUNT), EINVAL);

    size_t result_count;
    CU_ASSERT_EQUAL(get(NULL, &result_count), EINVAL);
    CU_ASSERT_EQUAL(get(attr, NULL), EINVAL);
}
#define ATTR_COUNT_TEST_FAIL(name, attr, bad_count)                            \
    attr_count_test_fail(ATTR_SET(name), ATTR_GET(name), attr, bad_count)

void attr_count_test_pass(attr_count_set set, attr_count_get get,
                          threadpool_attr_t *attr, size_t value) {
    size_t result_count = 0;
    CU_ASSERT_EQUAL(set(attr, value), SUCCESS);
    CU_ASSERT_EQUAL(get(attr, &result_count), SUCCESS);
    CU_ASSERT_EQUAL(result_count, value);
}
#define ATTR_COUNT_TEST_PASS(name, attr, value)                                \
    attr_count_test_pass(ATTR_SET(name), ATTR_GET(name), attr, value)

void attr_timeout_test_fail(attr_timeout_set set, attr_timeout_get get,
                            threadpool_attr_t *attr, time_t bad_timeout) {
    CU_ASSERT_EQUAL(set(attr, bad_timeout), EINVAL);
    CU_ASSERT_EQUAL(set(NULL, GOOD_COUNT), EINVAL);

    time_t result_timeout;
    CU_ASSERT_EQUAL(get(NULL, &result_timeout), EINVAL);
    CU_ASSERT_EQUAL(get(attr, NULL), EINVAL);
}
#define ATTR_TIMEOUT_TEST_FAIL(name, attr, bad_timeout)                        \
    attr_timeout_test_fail(ATTR_SET(name), ATTR_GET(name), attr, bad_timeout)

void attr_timeout_test_pass(attr_timeout_set set, attr_timeout_get get,
                            threadpool_attr_t *attr, time_t value) {
    time_t result_timeout = 0;
    CU_ASSERT_EQUAL(set(attr, value), SUCCESS);
    CU_ASSERT_EQUAL(get(attr, &result_timeout), SUCCESS);
    CU_ASSERT_EQUAL(result_timeout, value);
}
#define ATTR_TIMEOUT_TEST_PASS(name, attr, value)                              \
    attr_timeout_test_pass(ATTR_SET(name), ATTR_GET(name), attr, value)

/**
 * @brief Set up the test object
 *
 * This function is called before each test function is called. It sets up the
 * threadpool for testing.
 *
 * @return threadpool_t* pointer to threadpool_t
 */
threadpool_t *setup_test(threadpool_attr_t **attr) {
    if (*attr == NULL) {
        *attr = threadpool_attr_init();
        CU_ASSERT_PTR_NOT_NULL_FATAL(*attr);
        CU_ASSERT_EQUAL_FATAL(
            threadpool_attr_set_thread_count(*attr, THREAD_COUNT), SUCCESS);
        CU_ASSERT_EQUAL_FATAL(threadpool_attr_set_queue_size(*attr, QUEUE_SIZE),
                              SUCCESS);
        CU_ASSERT_EQUAL_FATAL(
            threadpool_attr_set_block_on_add(*attr, BLOCK_ON_ADD_ENABLED),
            SUCCESS);
        CU_ASSERT_EQUAL_FATAL(
            threadpool_attr_set_timed_wait(*attr, TIMED_WAIT_ENABLED), SUCCESS);
        CU_ASSERT_EQUAL_FATAL(threadpool_attr_set_timeout(*attr, TIMEOUT),
                              SUCCESS);
    }
    fprintf(stderr, "\tsetup_test\n");
    threadpool_t *pool = threadpool_create(*attr, NULL);
    CU_ASSERT_PTR_NOT_NULL_FATAL(pool);
    fprintf(stderr, "\tsetup_test done\n");
    return pool;
}

/**
 * @brief Tear down the test object
 *
 * This function is called after each test function is called. It destroys the
 * threadpool.
 *
 * @param pool pointer to threadpool_t
 */
void teardown_test(threadpool_t **pool, threadpool_attr_t *attr) {
    fprintf(stderr, "\tteardown_test\n");
    CU_ASSERT_EQUAL(threadpool_destroy(*pool, SHUTDOWN_GRACEFUL), SUCCESS);
    *pool = NULL;
    CU_ASSERT_EQUAL(threadpool_attr_destroy(attr), SUCCESS);
    fprintf(stderr, "\tteardown_test done\n");
}

int dummy_task(void *arg, void *arg2) {
    (void)arg2;
    fprintf(stderr, "\ton thread %lX: dummy_task\n", pthread_self());
    sleep(TIMEOUT);
    fprintf(stderr, "\ton thread %lX: work starting\n", pthread_self());
    int *done = arg;
    pthread_mutex_lock(&lock);
    (*done)++;
    fprintf(stderr, "\ton thread %lX: %d task(s) done\n", pthread_self(),
            *done);
    pthread_mutex_unlock(&lock);
    return SUCCESS;
}

/**
 * @brief Start tasks in the threadpool
 *
 * This function starts tasks in the threadpool until the queue is full.
 *
 * @param pool pointer to threadpool_t
 * @param done pointer to int for dummy task to increment
 * @return int number of tasks started
 */
int start_tasks(threadpool_t *pool, int *done) {
    int tasks = 0;
    int work_added;
    do {
        work_added =
            threadpool_timed_add_work(pool, dummy_task, done, NULL, TIMEOUT);
        switch (work_added) {
        case SUCCESS:
            tasks++;
            break;
        // case EAGAIN:
        case ETIMEDOUT:
            // adding work failed in an expected way
            break;
        default:
            fprintf(stderr, "\tthreadpool_add_work() return: %d\n", work_added);
            CU_ASSERT_EQUAL(work_added, SUCCESS);
        }
    } while (work_added == SUCCESS);
    return tasks;
}

void test_threadpool_attributes() {
    fprintf(stderr, "\ntest_threadpool_attributes start\n");
    // Test threadpool_attr_init()
    threadpool_attr_t *attr = threadpool_attr_init();
    CU_ASSERT_PTR_NOT_NULL_FATAL(attr);

    // Test threadpool_attr_t cancel_type
    ATTR_FLAG_TEST_FAIL(cancel_type, attr);
    ATTR_FLAG_TEST_PASS(cancel_type, attr, CANCEL_ASYNC);
    ATTR_FLAG_TEST_PASS(cancel_type, attr, CANCEL_DEFERRED);

    // Test threadpool_attr_t timed_wait/timeout
    ATTR_FLAG_TEST_FAIL(timed_wait, attr);
    ATTR_FLAG_TEST_PASS(timed_wait, attr, TIMED_WAIT_ENABLED);
    ATTR_FLAG_TEST_PASS(timed_wait, attr, TIMED_WAIT_DISABLED);
    ATTR_TIMEOUT_TEST_FAIL(timeout, attr, 0);
    ATTR_TIMEOUT_TEST_PASS(timeout, attr, TIMEOUT);

    // Test threadpool_attr_t block_on_add
    ATTR_FLAG_TEST_FAIL(block_on_add, attr);
    ATTR_FLAG_TEST_PASS(block_on_add, attr, BLOCK_ON_ADD_ENABLED);
    ATTR_FLAG_TEST_PASS(block_on_add, attr, BLOCK_ON_ADD_DISABLED);

    // Test threadpool_attr_t thread_count
    ATTR_COUNT_TEST_FAIL(thread_count, attr, 0);
    ATTR_COUNT_TEST_FAIL(thread_count, attr, MAX_THREADS + 1);
    ATTR_COUNT_TEST_PASS(thread_count, attr, THREAD_COUNT);

    // Test threadpool_attr_t queue_size
    ATTR_COUNT_TEST_FAIL(queue_size, attr, 0);
    ATTR_COUNT_TEST_PASS(queue_size, attr, QUEUE_SIZE);

    // Test threadpool_attr_destroy()
    CU_ASSERT_NOT_EQUAL(threadpool_attr_destroy(NULL), SUCCESS);
    CU_ASSERT_EQUAL(threadpool_attr_destroy(attr), SUCCESS);
    fprintf(stderr, "test_threadpool_attributes done\n\n");
}

void test_threadpool_run_tasks() {
    fprintf(stderr, "test_threadpool_run_tasks start\n");
    threadpool_attr_t *attr = NULL;

    threadpool_t *pool = setup_test(&attr);

    // Confirm that threadpool_add_work() returns EINVAL when pool or action are
    // NULL
    CU_ASSERT_EQUAL(threadpool_add_work(pool, NULL, NULL, NULL), EINVAL);
    CU_ASSERT_EQUAL(threadpool_add_work(NULL, dummy_task, NULL, NULL), EINVAL);
    // Add QUEUE_SIZE tasks to the threadpool. Confirm that
    // threadpool_add_work() returns EAGAIN when the queue is full
    int done = 0;
    pthread_mutex_lock(&lock);
    int tasks = start_tasks(pool, &done);
    pthread_mutex_unlock(&lock);

    // add THREAD_COUNT because the first tasks are started, freeing spots in
    // the queue
    fprintf(stderr, "\ttask count: %d\texpected: %d\n", tasks,
            QUEUE_SIZE + THREAD_COUNT);
    CU_ASSERT_EQUAL(tasks, QUEUE_SIZE + THREAD_COUNT);
    // Confirm that threadpool_wait() returns EINVAL when pool is NULL
    CU_ASSERT_EQUAL(threadpool_wait(NULL), EINVAL);
    CU_ASSERT_EQUAL(threadpool_timed_wait(NULL, TIMEOUT), EINVAL);
    CU_ASSERT_EQUAL(threadpool_timed_wait(pool, 0), EINVAL);
    // Confirm that threadpool_timed_wait() successfully waits for all tasks to
    // finish
    CU_ASSERT_EQUAL(threadpool_timed_wait(pool, TIMEOUT * tasks), SUCCESS);
    // Confirm that all tasks were completed
    CU_ASSERT_EQUAL(done, tasks);

    teardown_test(&pool, attr);
    fprintf(stderr, "test_threadpool_run_tasks done\n\n");
}

void test_threadpool_shutdown() {
    fprintf(stderr, "test_threadpool_shutdown start\n");
    threadpool_attr_t *attr = NULL;
    threadpool_t *pool = setup_test(&attr);

    // Confirm that threadpool_destroy() returns EINVAL when pool is NULL
    CU_ASSERT_EQUAL(threadpool_destroy(NULL, SHUTDOWN_GRACEFUL), EINVAL);

    // Confirm that threadpool_destroy() forcefully destroys the threadpool
    fprintf(stderr, "\tQUIT_FORCE\n");
    int done = 0;
    pthread_mutex_lock(&lock);
    int tasks = start_tasks(pool, &done);
    pthread_mutex_unlock(&lock);
    CU_ASSERT_EQUAL(threadpool_destroy(pool, SHUTDOWN_FORCEFUL), SUCCESS);
    sleep(1); // allow all threads to shut down
    CU_ASSERT_NOT_EQUAL(done, tasks);

    // Confirm that threadpool_destroy() gracefully destroys the threadpool
    fprintf(stderr, "\tQUIT_GRACEFUL\n");
    pool = setup_test(&attr);
    CU_ASSERT_PTR_NOT_NULL_FATAL(pool);
    sleep(1); // allow all threads to start up
    done = 0;
    pthread_mutex_lock(&lock);
    tasks = start_tasks(pool, &done);
    pthread_mutex_unlock(&lock);
    teardown_test(&pool, attr);
    CU_ASSERT_EQUAL(done, tasks);
    fprintf(stderr, "test_threadpool_shutdown done\n\n");
}

int main(void) {
    CU_TestInfo suite1_tests[] = {
        {"Testing threadpool attributes:", test_threadpool_attributes},
        {"Testing threadpool tasks:", test_threadpool_run_tasks},
        {"Testing threadpool shutdown:", test_threadpool_shutdown},

        CU_TEST_INFO_NULL};

    CU_SuiteInfo suites[] = {
        {"Suite-1:", init_suite1, clean_suite1, .pTests = suite1_tests},
        CU_SUITE_INFO_NULL};

    if (CUE_SUCCESS != CU_initialize_registry()) {
        return CU_get_error();
    }

    if (0 != CU_register_suites(suites)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_basic_show_failures(CU_get_failure_list());
    int num_failed = CU_get_number_of_failures();
    CU_cleanup_registry();
    printf("\n");
    return num_failed;
}
