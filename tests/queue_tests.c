#include "queue.h"
#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>
#include <stdlib.h>

enum {
    SUCCESS = 0,
    CAPACITY = 5,
};

// The queue to be used by all the tests
queue_t *queue = NULL;
// The integer all node data[5] pointers point to
int data[] = {1, 2, 3, 4, 5};

int init_suite1(void) { return 0; }

int clean_suite1(void) { return 0; }

/**
 * @brief frees an item and its associated memory
 *
 * @param mem_addr pointer of the item to be free'd
 */
void custom_free(void *mem_addr) {}

int test_compare_node(const void *value_to_find, const void *node_data) {
    int value_to_find_int = value_to_find == NULL ? -1 : *(int *)value_to_find;
    int node_data_int = node_data == NULL ? -1 : *(int *)node_data;

    if (node_data_int > value_to_find_int) {
        return -1;
    } else if (node_data_int < value_to_find_int) {
        return 1;
    } else {
        return 0;
    }
}

void test_queue_init() {
    // Verify queue was created correctly
    queue = queue_init(CAPACITY, custom_free, test_compare_node, NULL);
    CU_ASSERT_PTR_NOT_NULL(queue);
    CU_ASSERT_EQUAL(queue_capacity(queue), CAPACITY);
    CU_ASSERT_EQUAL(queue_size(queue), 0);
}

void test_queue_enqueue() {
    queue_t *invalid_queue = NULL;

    // Should catch if enqueue is called on an invalid queue or with invalid
    // data
    int exit_code = queue_enqueue(invalid_queue, &data[0]);
    CU_ASSERT_NOT_EQUAL(exit_code, SUCCESS);

    CU_ASSERT_PTR_NOT_NULL_FATAL(queue);
    // enqueue CAPACITY number of nodes
    for (size_t i = 0; i < CAPACITY; i++) {
        exit_code = queue_enqueue(queue, &data[i]);
        // Function exited correctly
        CU_ASSERT_EQUAL(exit_code, SUCCESS);
    }

    // queue size is correct
    CU_ASSERT_EQUAL(queue_size(queue), CAPACITY);

    // Function should return a code if enqueue is called on a full queue
    exit_code = queue_enqueue(queue, &data[5]);
    CU_ASSERT_NOT_EQUAL(exit_code, SUCCESS);
}

void test_queue_dequeue() {
    queue_t *invalid_queue = NULL;

    // Should catch if enqueue is called on an invalid queue
    void *node = queue_dequeue(invalid_queue);
    CU_ASSERT_PTR_NULL(node);

    CU_ASSERT_PTR_NOT_NULL_FATAL(queue);
    // Dequeue all items
    for (size_t i = 0; queue_size(queue) > 0; i++) {
        node = queue_dequeue(queue);
        CU_ASSERT_PTR_NOT_NULL_FATAL(node);
        CU_ASSERT_PTR_EQUAL(node, &data[i]);
    }

    // test adding less than the queue's total capacity before removing items
    queue_enqueue(queue, &data[0]);
    queue_enqueue(queue, &data[1]);

    node = queue_dequeue(queue);
    CU_ASSERT_PTR_NOT_NULL_FATAL(node);
    CU_ASSERT_EQUAL(*(int *)node, data[0]);

    node = queue_dequeue(queue);
    CU_ASSERT_PTR_NOT_NULL_FATAL(node);
    CU_ASSERT_EQUAL(*(int *)node, data[1]);

    // test adding and removing a single item, twice
    queue_enqueue(queue, &data[0]);
    node = queue_dequeue(queue);
    CU_ASSERT_PTR_NOT_NULL_FATAL(node);
    CU_ASSERT_EQUAL(*(int *)node, data[0]);

    queue_enqueue(queue, &data[1]);
    node = queue_dequeue(queue);
    CU_ASSERT_PTR_NOT_NULL_FATAL(node);
    CU_ASSERT_EQUAL(*(int *)node, data[1]);

    // Should return NULL when called on empty queue
    node = queue_dequeue(queue);
    CU_ASSERT_PTR_NULL(node);
}

void test_queue_peek() {
    queue_t *invalid_queue = NULL;

    // Should catch if peek is called on an invalid queue or empty
    void *node = queue_peek(invalid_queue);
    CU_ASSERT_PTR_NULL(node);

    CU_ASSERT_PTR_NOT_NULL_FATAL(queue);
    node = queue_peek(queue);
    CU_ASSERT_PTR_NULL(node); // queue is empty

    // enqueue CAPACITY number of nodes
    for (size_t i = 0; i < CAPACITY; i++) {
        queue_enqueue(queue, &data[i]);
    }

    node = queue_peek(queue);

    // Function should have exited successfully
    CU_ASSERT_PTR_NOT_NULL_FATAL(node);
    // Correct value should have been peeked from front node
    CU_ASSERT_EQUAL(*(int *)node, data[0]);
    // Size shouldn't have changed
    CU_ASSERT_EQUAL(queue_size(queue), CAPACITY);
}

void test_queue_get() {
    queue_t *invalid_queue = NULL;

    // Should catch if get is called on an invalid queue
    void *node = queue_get(invalid_queue, 0);
    CU_ASSERT_PTR_NULL(node);

    CU_ASSERT_PTR_NOT_NULL_FATAL(queue);
    // Should catch if get is called outside of the queue's bounds
    node = queue_get(queue, CAPACITY + 1);
    CU_ASSERT_PTR_NULL(node);

    // check random positions in the queue
    size_t indexes[] = {0, 3, 1, 4, 2};
    for (size_t i = 0; i < CAPACITY; i++) {
        node = queue_get(queue, indexes[i]);
        CU_ASSERT_PTR_NOT_NULL_FATAL(node);
        CU_ASSERT_EQUAL(*(int *)node, data[indexes[i]]);
    }

    // get does not remove the node from the queue
    CU_ASSERT_EQUAL(queue_size(queue), CAPACITY);
}

void test_queue_find_first() {
    queue_t *invalid_queue = NULL;

    // Should catch if find is called on an invalid queue
    void *node = queue_find_first(invalid_queue, NULL);
    CU_ASSERT_PTR_NULL(node);

    CU_ASSERT_PTR_NOT_NULL_FATAL(queue);
    // Should catch if find is called on NULL data
    node = queue_find_first(queue, NULL);
    CU_ASSERT_PTR_NULL(node);

    // check random positions in the queue
    size_t indexes[] = {0, 3, 1, 4, 2};
    for (size_t i = 0; i < CAPACITY; i++) {
        node = queue_find_first(queue, &data[indexes[i]]);
        CU_ASSERT_PTR_NOT_NULL_FATAL(node);
        CU_ASSERT_EQUAL(*(int *)node, data[indexes[i]]);
    }

    // find does not remove the node from the queue
    CU_ASSERT_EQUAL(queue_size(queue), CAPACITY);
}

void test_queue_remove() {
    queue_t *invalid_queue = NULL;

    // Should catch if find is called on an invalid queue
    void *node = queue_remove(invalid_queue, NULL);
    CU_ASSERT_PTR_NULL(node);

    CU_ASSERT_PTR_NOT_NULL_FATAL(queue);
    // Should catch if remove is called on NULL data
    node = queue_remove(queue, NULL);
    CU_ASSERT_PTR_NULL(node);

    // check random positions in the queue
    size_t indexes[] = {0, 3, 1, 4, 2};
    for (size_t i = 0; i < CAPACITY; i++) {
        node = queue_remove(queue, &data[indexes[i]]);
        CU_ASSERT_PTR_NOT_NULL_FATAL(node);
        CU_ASSERT_EQUAL(*(int *)node, data[indexes[i]]);
        // confirm removal
        node = queue_find_first(queue, &data[indexes[i]]);
        CU_ASSERT_PTR_NULL(node);
    }

    // enqueue CAPACITY number of nodes
    for (size_t i = 0; i < CAPACITY; i++) {
        queue_enqueue(queue, &data[i]);
    }
}

void test_queue_clear() {
    queue_t *invalid_queue = NULL;

    // Should catch if clear is called on an invalid queue
    int exit_code = queue_clear(invalid_queue);
    CU_ASSERT_NOT_EQUAL(exit_code, SUCCESS);

    CU_ASSERT_PTR_NOT_NULL_FATAL(queue);
    exit_code = queue_clear(queue);
    // Function should have exited fully
    CU_ASSERT_EQUAL(exit_code, SUCCESS);
    // queue should now be empty
    CU_ASSERT_EQUAL(queue_size(queue), 0);
}

void test_queue_destroy() {
    queue_t *invalid_queue = NULL;

    // Should catch if delete is called on an invalid queue
    int exit_code = queue_destroy(&invalid_queue);
    CU_ASSERT_NOT_EQUAL(exit_code, SUCCESS);

    CU_ASSERT_PTR_NOT_NULL_FATAL(queue);
    // Function should have exited fully
    exit_code = queue_destroy(&queue);
    CU_ASSERT_EQUAL(exit_code, SUCCESS);

    // Should catch if delete is called on the queue that has already been
    // deleted
    exit_code = queue_destroy(&queue);
    CU_ASSERT_NOT_EQUAL(exit_code, SUCCESS);
}

int main(void) {
    CU_TestInfo suite1_tests[] = {
        {"Testing queue_init():", test_queue_init},

        {"Testing queue_enqueue():", test_queue_enqueue},

        {"Testing queue_dequeue():", test_queue_dequeue},

        {"Testing queue_peek():", test_queue_peek},

        {"Testing queue_get():", test_queue_get},

        {"Testing queue_find_first():", test_queue_find_first},

        {"Testing queue_clear():", test_queue_clear},

        {"Testing queue_destroy():", test_queue_destroy},
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
