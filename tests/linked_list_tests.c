#include "linked_list.h"
#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>
#include <stdio.h>
#include <stdlib.h>

// The integer all node data[5] pointers point to
int data[] = {1, 2, 3, 4, 5};

#define SIZE sizeof(data) / sizeof(*data)
#define INVALID_LIST NULL
#define SUCCESS 0

// The list to be used by all the tests
list_t *list = NULL;
list_t *empty = NULL;
list_t *no_cmp = NULL;

int value_to_remove;

int test_compare_node(const void *value_to_find, const void *node_data) {
    int value_to_find_int = *(int *)value_to_find;
    int node_data_int = *(int *)node_data;
    if (node_data_int > value_to_find_int) {
        return -1;
    } else if (node_data_int < value_to_find_int) {
        return 1;
    } else {
        return 0;
    }
}

/**
 * @brief This function is used to modify the data in the list
 *
 * Set the data to the remainder of the data and 2
 *
 * @param data The data to be modified
 * @param addl_data unused
 * @return int 0 if the function exited successfully
 */
int make_mod(void **data, void *addl_data) {
    (void)addl_data;
    int number = **(int **)data;
    **(int **)data = number % 2;
    return SUCCESS;
}

void custom_free(void *mem_addr) { (void)mem_addr; }

int init_suite1(void) { return 0; }

int clean_suite1(void) { return 0; }

void test_list_new() {
    // Verify lists were created correctly
    no_cmp = list_new(NULL, NULL, NULL);
    CU_ASSERT_PTR_NOT_NULL_FATAL(no_cmp); // Function exited correctly
    empty = list_new(NULL, test_compare_node, NULL);
    CU_ASSERT_PTR_NOT_NULL_FATAL(empty); // Function exited correctly
    list = list_new(custom_free, test_compare_node, NULL);
    CU_ASSERT_PTR_NOT_NULL_FATAL(list);    // Function exited correctly
    CU_ASSERT_EQUAL(list_size(no_cmp), 0); // list size is correct
    CU_ASSERT_EQUAL(list_size(empty), 0);  // list size is correct
    CU_ASSERT_EQUAL(list_size(list), 0);   // list size is correct
}

void test_list_push_tail() {
    // Should catch if push is called on an invalid list
    CU_ASSERT_EQUAL(list_push_tail(INVALID_LIST, &data[0]), EINVAL);

    CU_ASSERT_PTR_NOT_NULL_FATAL(list);
    int exit_code = 1;
    for (size_t i = 0; i < SIZE; i++) {
        exit_code = list_push_tail(list, &data[i]);
        if (exit_code != SUCCESS) {
            break;
        }
        // New node was pushed and points to the correct data
        CU_ASSERT_EQUAL(*(int *)(list_peek_tail(list)), data[i]);
    }

    // Function exited correctly
    CU_ASSERT_EQUAL(exit_code, SUCCESS);
    // list size is correct
    CU_ASSERT_EQUAL(list_size(list), SIZE);
}

void test_list_pop_head() {
    // Should catch if pop is called on an invalid list
    CU_ASSERT_PTR_NULL(list_pop_head(INVALID_LIST));

    CU_ASSERT_PTR_NOT_NULL_FATAL(list);
    // Pop all nodes
    ssize_t cur_size = list_size(list);
    for (ssize_t i = 0; i < cur_size; i++) {
        int *node_data = list_pop_head(list);
        CU_ASSERT_PTR_NOT_NULL_FATAL(node_data);
        CU_ASSERT_EQUAL(*node_data, data[i]);
    }
    CU_ASSERT_EQUAL(list_size(list), 0);
    // Function should return NULL if pop is called on an empty list
    CU_ASSERT_PTR_NULL(list_pop_head(list));
}

void test_list_push_head() {
    // Should catch if push is called on an invalid list
    CU_ASSERT_EQUAL(list_push_head(INVALID_LIST, &data[0]), EINVAL);

    // Push SIZE nodes
    CU_ASSERT_PTR_NOT_NULL_FATAL(list);
    int exit_code = 1;
    for (size_t i = 0; i < SIZE; i++) {
        exit_code = list_push_head(list, &data[i]);
        if (exit_code != SUCCESS) {
            break;
        }
        // New node was pushed and points to the correct data
        CU_ASSERT_EQUAL(*(int *)(list_peek_head(list)), data[i]);
    }

    // Function exited correctly
    CU_ASSERT_EQUAL(exit_code, SUCCESS);
    // list size is correct
    CU_ASSERT_EQUAL(list_size(list), SIZE);
}

void test_list_sort() {
    // Should catch if sort is called on an invalid list
    CU_ASSERT_EQUAL(list_sort(INVALID_LIST), EINVAL);
    CU_ASSERT_EQUAL(list_sort(no_cmp), ENOTSUP);

    // Ensure function exited successfully
    CU_ASSERT_PTR_NOT_NULL_FATAL(list);
    CU_ASSERT_EQUAL(list_sort(list), SUCCESS);

    // Verify list should now be reversed
    ssize_t cur_size = list_size(list);
    for (ssize_t i = 0; i < cur_size; i++) {
        int *node_data = list_pop_head(list);
        CU_ASSERT_PTR_NOT_NULL_FATAL(node_data);
        // Correct value should be successfully popped from head
        CU_ASSERT_EQUAL(*node_data, data[i]);
    }

    // Return the list to its original state for following tests
    for (ssize_t i = 0; i < cur_size; i++) {
        list_push_head(list, &data[i]);
    }
}

void test_list_pop_tail() {
    // Should catch if pop is called on an invalid list
    CU_ASSERT_PTR_NULL(list_pop_tail(INVALID_LIST));

    // Pop all nodes
    CU_ASSERT_PTR_NOT_NULL_FATAL(list);
    ssize_t cur_size = list_size(list);
    for (ssize_t i = 0; i < cur_size; i++) {
        int *node_data = list_pop_tail(list);
        // Correct value should be successfully popped into value_from_node
        CU_ASSERT_PTR_NOT_NULL_FATAL(node_data);
        CU_ASSERT_EQUAL(*node_data, data[i]);
    }

    CU_ASSERT_EQUAL(list_size(list), 0);

    // Function should return null if pop is called on an empty list
    CU_ASSERT_PTR_NULL(list_pop_tail(list));
}

void test_list_peek_head() {
    // Should catch if function is called on an invalid list
    CU_ASSERT_PTR_NULL(list_peek_head(INVALID_LIST));

    // Function should not be able to peek on an empty list
    CU_ASSERT_PTR_NOT_NULL_FATAL(list);
    CU_ASSERT_PTR_NULL(list_peek_head(list));

    // Push the nodes tail into list
    for (size_t i = 0; i < SIZE; i++) {
        list_push_tail(list, &data[i]);
    }

    ssize_t cur_size = list_size(list);
    int *node_data = list_peek_head(list);
    // Function should have exited successfully
    CU_ASSERT_PTR_NOT_NULL_FATAL(node_data);
    // Correct value should have been peeked from head node
    CU_ASSERT_EQUAL(*node_data, data[0]);
    // Size shouldn't have changed
    CU_ASSERT_EQUAL(list_size(list), cur_size);
}

void test_list_iterator() {
    // Should catch if function is called on an invalid list
    CU_ASSERT_EQUAL(list_iterator_reset(INVALID_LIST), EINVAL);
    int err = SUCCESS;
    CU_ASSERT_PTR_NULL(list_iterator_next(INVALID_LIST, &err));
    CU_ASSERT_EQUAL(err, EINVAL);

    CU_ASSERT_PTR_NOT_NULL_FATAL(list);
    ssize_t cur_size = list_size(list);
    // Confirm iterator is iterating correctly
    CU_ASSERT_EQUAL(list_iterator_reset(list), SUCCESS);
    for (ssize_t i = 0; i < cur_size; i++) {
        CU_ASSERT_PTR_EQUAL(list_iterator_next(list, NULL), &data[i]);
    }
    CU_ASSERT_PTR_NULL(list_iterator_next(list, NULL));

    // Confirm iterator is resetting correctly
    CU_ASSERT_EQUAL(list_iterator_reset(list), SUCCESS);
    CU_ASSERT_PTR_EQUAL(list_iterator_next(list, NULL), &data[0]);

    // Size shouldn't have changed
    CU_ASSERT_EQUAL(list_size(list), cur_size);
}

void test_list_remove() {
    // Should catch if remove is called on an invalid list
    int err = SUCCESS;
    CU_ASSERT_PTR_NULL(list_remove(INVALID_LIST, NULL, &err));
    CU_ASSERT_EQUAL(err, EINVAL);
    err = SUCCESS;
    CU_ASSERT_PTR_NULL(list_remove(no_cmp, NULL, &err));
    CU_ASSERT_EQUAL(err, ENOTSUP);

    CU_ASSERT_PTR_NOT_NULL_FATAL(list);
    value_to_remove = data[SIZE - 2];
    ssize_t cur_size = list_size(list);
    int *removed_value = list_remove(list, &value_to_remove, NULL);
    // Function should have exited successfully
    CU_ASSERT_EQUAL(*removed_value, value_to_remove);
    // Size should reflect the removal of the node
    CU_ASSERT_EQUAL(list_size(list), cur_size - 1);

    // The node containing the removed value should no longer be in the list
    CU_ASSERT_PTR_NULL(list_remove(list, &value_to_remove, NULL));
}

void test_list_find_first() {
    // Should catch if function is called on an invalid list
    int err = SUCCESS;
    CU_ASSERT_PTR_NULL(list_find_first(INVALID_LIST, NULL, &err));
    CU_ASSERT_EQUAL(err, EINVAL);
    err = SUCCESS;
    CU_ASSERT_PTR_NULL(list_find_first(no_cmp, NULL, &err));
    CU_ASSERT_EQUAL(err, ENOTSUP);

    // Should not be able to find the value removed in the previous test
    CU_ASSERT_PTR_NOT_NULL_FATAL(list);
    CU_ASSERT_PTR_NULL(list_find_first(list, &value_to_remove, NULL));

    // Change the value we are looking for to one that is still in the list
    value_to_remove = data[SIZE - 1];
    int *node_data = list_find_first(list, &value_to_remove, NULL);
    // Ensure function exited successfully
    CU_ASSERT_PTR_NOT_NULL_FATAL(node_data);
    // Ensure the value found was the one that was searched for
    CU_ASSERT_EQUAL(*node_data, value_to_remove);
}

void test_list_foreach_call() {
    // Should catch if function is called on an invalid list
    CU_ASSERT_EQUAL(list_foreach_call(INVALID_LIST, make_mod, NULL), EINVAL);

    // Ensure function exited correctly
    CU_ASSERT_PTR_NOT_NULL_FATAL(list);
    CU_ASSERT_EQUAL(list_foreach_call(list, make_mod, NULL), SUCCESS);

    // Ensure the user defined action was done to the list nodes
    CU_ASSERT_EQUAL(*(int *)list_get(list, 0), 1);
    CU_ASSERT_EQUAL(*(int *)list_get(list, 1), 0);
}

void test_list_find_all() {
    // Should catch if function is called on an invalid list
    int err = SUCCESS;
    CU_ASSERT_PTR_NULL(list_find_all(INVALID_LIST, NULL, &err));
    CU_ASSERT_EQUAL(err, EINVAL);
    err = SUCCESS;
    CU_ASSERT_PTR_NULL(list_find_all(no_cmp, NULL, &err));
    CU_ASSERT_EQUAL(err, ENOTSUP);

    // Should catch if function is called on an empty list
    CU_ASSERT_PTR_NOT_NULL_FATAL(empty);
    int value_to_find = 1;
    CU_ASSERT_PTR_NULL(list_find_all(empty, &value_to_find, NULL));

    CU_ASSERT_PTR_NOT_NULL_FATAL(list);
    list_t *result_list = list_find_all(list, &value_to_find, NULL);
    // Ensure function exited successfully
    CU_ASSERT_PTR_NOT_NULL_FATAL(result_list);
    // Verify that each node of the resulting list is the correct value
    for (ssize_t i = 0; i < list_size(result_list); i++) {
        int *num = list_get(result_list, i);
        CU_ASSERT_PTR_NOT_NULL_FATAL(num);
        CU_ASSERT_EQUAL(*num, value_to_find);
    }

    // Verify that the correct number of occurrences were found
    CU_ASSERT_EQUAL(list_size(result_list), 3);
    list_delete(&result_list);
}

void test_list_get() {
    size_t position = 2;

    // Should catch if get is called on an invalid list
    CU_ASSERT_PTR_NULL(list_get(INVALID_LIST, position));

    // Should catch if get is called on an empty list
    CU_ASSERT_PTR_NOT_NULL_FATAL(empty);
    CU_ASSERT_PTR_NULL(list_get(empty, position));

    CU_ASSERT_PTR_NOT_NULL_FATAL(list);
    ssize_t cur_size = list_size(list);
    int *node = list_get(list, position);
    // Function should have exited successfully
    CU_ASSERT_PTR_NOT_NULL_FATAL(node);
    // Correct value should have been peeked from head node
    CU_ASSERT_EQUAL(*node, data[position]);
    // Size shouldn't have changed
    CU_ASSERT_EQUAL(list_size(list), cur_size);
}

void test_list_insert() {
    int value_to_insert = 9;
    size_t position = 2;

    // Should catch if insert is called on an invalid list
    CU_ASSERT_EQUAL(list_insert(INVALID_LIST, &value_to_insert, position),
                    EINVAL);

    CU_ASSERT_PTR_NOT_NULL_FATAL(list);
    ssize_t cur_size = list_size(list);
    // Function should have exited successfully
    CU_ASSERT_EQUAL(list_insert(list, &value_to_insert, position), SUCCESS);
    // Size should reflect the insertion of the node
    CU_ASSERT_EQUAL(list_size(list), cur_size + 1);

    // The node containing the inserted value should now be in the list
    int *node_data = list_get(list, position);
    CU_ASSERT_PTR_NOT_NULL_FATAL(node_data);
    CU_ASSERT_EQUAL(*node_data, value_to_insert);
}

void test_list_clear() {
    // Should catch if clear is called on an invalid list
    CU_ASSERT_EQUAL(list_clear(INVALID_LIST), EINVAL);

    CU_ASSERT_PTR_NOT_NULL_FATAL(list);
    // list should now be empty
    CU_ASSERT_PTR_NOT_NULL(list_peek_head(list));
    CU_ASSERT_EQUAL(list_clear(list), SUCCESS)
    CU_ASSERT_PTR_NULL(list_peek_head(list));
}

void test_list_peek_tail() {
    // Should catch if pop is called on an invalid list
    CU_ASSERT_PTR_NULL(list_peek_tail(INVALID_LIST));

    // Function should not be able to peek on an empty list
    CU_ASSERT_PTR_NOT_NULL_FATAL(list);
    CU_ASSERT_PTR_NULL(list_peek_tail(list));

    // Push the nodes tail into list
    for (size_t i = 0; i < SIZE; i++) {
        list_push_tail(list, &data[i]);
    }

    int *node_data = list_peek_tail(list);
    // Function should have exited successfully
    CU_ASSERT_PTR_NOT_NULL_FATAL(node_data);
    // Correct value should have been peeked from head node
    CU_ASSERT_EQUAL(*node_data, data[SIZE - 1]);
    // Size shouldn't have changed
    CU_ASSERT_EQUAL(list_size(list), SIZE);
}

void test_list_delete() {
    // Should catch if delete is called on an invalid list
    CU_ASSERT_EQUAL(list_delete(INVALID_LIST), EINVAL);

    // Function should have exited successfully
    CU_ASSERT_PTR_NOT_NULL_FATAL(list);
    CU_ASSERT_EQUAL(list_delete(&list), SUCCESS);
    CU_ASSERT_PTR_NOT_NULL_FATAL(empty);
    CU_ASSERT_EQUAL(list_delete(&empty), SUCCESS);
    CU_ASSERT_PTR_NOT_NULL_FATAL(no_cmp);
    CU_ASSERT_EQUAL(list_delete(&no_cmp), SUCCESS);

    // Should catch if delete is called on the list that has already been
    // deleted
    CU_ASSERT_EQUAL(list_delete(&list), EINVAL);
}

int main(void) {
    CU_TestInfo suite1_tests[] = {
        {"Testing list_new():", test_list_new},

        {"Testing list_push_tail():", test_list_push_tail},

        {"Testing list_pop_head():", test_list_pop_head},

        {"Testing list_push_head():", test_list_push_head},

        {"Testing list_sort():", test_list_sort},

        {"Testing list_pop_tail():", test_list_pop_tail},

        {"Testing list_peek_head():", test_list_peek_head},

        {"Testing list_iterator():", test_list_iterator},

        {"Testing list_remove():", test_list_remove},

        {"Testing list_find_first():", test_list_find_first},

        {"Testing list_foreach_call():", test_list_foreach_call},

        {"Testing list_find_all():", test_list_find_all},

        {"Testing list_get():", test_list_get},

        {"Testing list_insert():", test_list_insert},

        {"Testing list_clear():", test_list_clear},

        {"Testing list_peek_tail():", test_list_peek_tail},

        {"Testing list_delete():", test_list_delete},
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
