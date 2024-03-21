#include "queue_p.h"
#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>
#include <stdlib.h>

enum {
    CAPACITY = 10,
    SUCCESS = 0,
    PRIORITY_0 = 0,
    PRIORITY_1 = 1,
};

// The queue_p to be used by all the tests
queue_p_t *queue_p = NULL;
queue_p_t *INVALID_QUEUE_P = NULL;
// The integer all node data[5] pointers point to
int data[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
int priority_data[] = {1, 2, 3, 4, 5};

static void reset_queue(queue_p_t *queue) {
    queue_p_node_t *tmp = NULL;

    while (queue_p_size(queue_p) != 0) {
        tmp = queue_p_dequeue(queue);
        if (NULL != tmp) {
            free(tmp);
        }
    }

    for (size_t i = 0; i != CAPACITY; i++) {
        queue_p_enqueue(queue, &data[i], PRIORITY_0);
    }
}

void custom_free(void *mem_addr) { (void)mem_addr; }

int test_compare_node(const void *value_to_find, const void *node_data) {
    const int *needle = value_to_find;
    const queue_p_node_t *q_node = node_data;
    if (*(int *)q_node->data > *needle) {
        return -1;
    } else if (*(int *)q_node->data < *needle) {
        return 1;
    } else {
        return 0;
    }
}

int init_suite1(void) { return 0; }

int clean_suite1(void) { return 0; }

void test_queue_p_init() {
    // Verify queue_p was created correctly
    queue_p = queue_p_init(CAPACITY, custom_free, test_compare_node, NULL);
    CU_ASSERT_FATAL(NULL != queue_p);
    // NOLINTNEXTLINE
    CU_ASSERT(CAPACITY == queue_p_capacity(queue_p));
    // NOLINTNEXTLINE
    CU_ASSERT(SUCCESS == queue_p_size(queue_p));
}

void test_queue_p_enqueue() {
    // Should catch if enqueue_p is called on an invalid queue_p or with invalid
    // data
    int exit_code = queue_p_enqueue(INVALID_QUEUE_P, &data[0], PRIORITY_0);
    CU_ASSERT(SUCCESS != exit_code);

    CU_ASSERT_FATAL(NULL != queue_p);
    // enqueue_p CAPACITY number of nodes
    for (size_t i = 0; i < CAPACITY / 2; i++) {
        exit_code = queue_p_enqueue(queue_p, &data[i], PRIORITY_0);
        // New node was enqueue_ped and points to the correct data
        CU_ASSERT_FATAL(SUCCESS == exit_code);
        queue_p_node_t *node = queue_p_get(queue_p, i * 2);
        CU_ASSERT_FATAL(NULL != node);
        CU_ASSERT(data[i] == *(int *)node->data);
        CU_ASSERT(PRIORITY_0 == node->priority);

        exit_code = queue_p_enqueue(queue_p, &priority_data[i], PRIORITY_1);
        CU_ASSERT_FATAL(SUCCESS == exit_code);
        node = queue_p_get(queue_p, i);
        CU_ASSERT_FATAL(NULL != node);
        CU_ASSERT(priority_data[i] == *(int *)node->data);
        CU_ASSERT(PRIORITY_1 == node->priority);
    }
    // queue_p size is correct
    CU_ASSERT(CAPACITY == queue_p_size(queue_p));
    // Function should return a code if enqueue_p is called on a full queue_p
    exit_code = queue_p_enqueue(queue_p, &data[5], PRIORITY_0);
    CU_ASSERT(SUCCESS != exit_code);
}

void test_queue_p_dequeue() {
    // Should catch if enqueue_p is called on an invalid queue_p
    queue_p_node_t *node = queue_p_dequeue(INVALID_QUEUE_P);
    CU_ASSERT(NULL == node);

    // Dequeue_p all items
    CU_ASSERT_FATAL(NULL != queue_p);
    reset_queue(queue_p);
    CU_ASSERT_FATAL(0 != queue_p_size(queue_p));
    for (size_t i = 0; queue_p_size(queue_p) > 0; i++) {
        node = queue_p_dequeue(queue_p);
        CU_ASSERT_FATAL(NULL != node);
        // NOLINTNEXTLINE
        CU_ASSERT(data[i] == *(int *)node->data);
        free(node);
        node = NULL;
    }

    // Should return NULL when called on empty queue_p
    node = queue_p_dequeue(queue_p);
    CU_ASSERT(NULL == node);
}

void test_queue_p_peek() {
    // Should catch if peek is called on an invalid queue_p or empty
    queue_p_node_t *node = queue_p_peek(INVALID_QUEUE_P);
    CU_ASSERT(NULL == node);

    CU_ASSERT_FATAL(NULL != queue_p);
    node = queue_p_peek(queue_p);
    CU_ASSERT(NULL == node);

    // enqueue_p CAPACITY number of nodes
    for (size_t i = 0; i < CAPACITY; i++) {
        queue_p_enqueue(queue_p, &data[i], PRIORITY_0);
    }

    node = queue_p_peek(queue_p);
    // Function should have exited successfully
    CU_ASSERT_FATAL(NULL != node);
    // Correct value should have been peeked from front node
    CU_ASSERT(data[0] == *(int *)node->data);
    // Size shouldn't have changed
    CU_ASSERT(CAPACITY == queue_p_size(queue_p));
}

void test_queue_p_get() {
    // Should catch if get is called on an invalid queue
    fprintf(stderr, "queue_p_get: testing invalid queue\n");
    queue_p_node_t *node = queue_p_get_priority(INVALID_QUEUE_P, 0, PRIORITY_0);
    CU_ASSERT(NULL == node);

    CU_ASSERT_FATAL(NULL != queue_p);
    // Should catch if get is called outside of the queue's bounds
    fprintf(stderr, "queue_p_get: testing out of bounds\n");
    node = queue_p_get_priority(queue_p, CAPACITY + 1, PRIORITY_0);
    CU_ASSERT(NULL == node);

    // check random positions in the queue
    size_t indexes[] = {9, 0, 5, 3, 1, 6, 4, 8, 2, 7};
    fprintf(stderr, "queue_p_get: testing random positions\n");
    for (size_t i = 0; i < CAPACITY; i++) {
        node = queue_p_get_priority(queue_p, indexes[i], PRIORITY_0);
        CU_ASSERT_FATAL(NULL != node);
        CU_ASSERT(data[indexes[i]] == *(int *)node->data);
    }

    // get does not remove the node from the queue
    CU_ASSERT(CAPACITY == queue_p_size(queue_p));
}

void test_queue_p_find_first() {
    // Should catch if find is called on an invalid queue
    queue_p_node_t *node = queue_p_find_first(INVALID_QUEUE_P, NULL);
    CU_ASSERT(NULL == node);

    CU_ASSERT_FATAL(NULL != queue_p);
    // check random positions in the queue
    size_t indexes[] = {9, 0, 5, 3, 1, 6, 4, 8, 2, 7};
    fprintf(stderr, "queue_p_find_first: testing random positions\n");
    for (size_t i = 0; i < CAPACITY; i++) {
        node = queue_p_find_first(queue_p, &data[indexes[i]]);
        CU_ASSERT_FATAL(NULL != node);
        CU_ASSERT(data[indexes[i]] == *(int *)node->data);
    }

    // find does not remove the node from the queue
    CU_ASSERT(CAPACITY == queue_p_size(queue_p));
}

void test_queue_p_clear() {
    // Should catch if clear is called on an invalid queue_p
    int exit_code = queue_p_clear(INVALID_QUEUE_P);
    CU_ASSERT(SUCCESS != exit_code);

    CU_ASSERT_FATAL(NULL != queue_p);
    exit_code = queue_p_clear(queue_p);
    // Function should have exited fully
    CU_ASSERT(SUCCESS == exit_code);
    // queue_p should now be empty
    CU_ASSERT(0 == queue_p_size(queue_p));
}

void test_queue_p_destroy() {
    // Should catch if delete is called on an invalid queue_p
    int exit_code = queue_p_destroy(&INVALID_QUEUE_P);
    CU_ASSERT(SUCCESS != exit_code);

    // Function should have exited fully
    CU_ASSERT_FATAL(NULL != queue_p);
    exit_code = queue_p_destroy(&queue_p);
    CU_ASSERT(SUCCESS == exit_code);

    // Should catch if delete is called on the queue_p that has already been
    // deleted
    exit_code = queue_p_destroy(&queue_p);
    CU_ASSERT(SUCCESS != exit_code);
}

int main(void) {
    CU_TestInfo suite1_tests[] = {
        {"Testing queue_p_init():", test_queue_p_init},

        {"Testing queue_p_enqueue():", test_queue_p_enqueue},

        {"Testing queue_p_dequeue():", test_queue_p_dequeue},

        {"Testing queue_p_peek():", test_queue_p_peek},

        {"Testing queue_get():", test_queue_p_get},

        {"Testing queue_find_first():", test_queue_p_find_first},

        {"Testing queue_p_clear():", test_queue_p_clear},

        {"Testing queue_p_destroy():", test_queue_p_destroy},
        CU_TEST_INFO_NULL};

    CU_SuiteInfo suites[] = {
        {"Suite-1:", init_suite1, clean_suite1, .pTests = suite1_tests},
        CU_SUITE_INFO_NULL};

    if (0 != CU_initialize_registry()) {
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
