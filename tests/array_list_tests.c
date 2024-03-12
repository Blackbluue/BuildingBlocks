#include "array_list.h"
#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

// The integer values for testing
int data[] = {9, 0, 7, 1, 5, 3, 2, 6, 8, 4};
void *wrapped = NULL;
int **int_arr;
enum {
    SIZE = sizeof(data) / sizeof(*data), // initial size of array
    SUCCESS = 0,                         // no error
    INVALID = -1,                        // invalid input
};
// The lists to be used by all the tests
arr_list_t *list = NULL;               // valid list
const arr_list_t *EMPTY_LIST = NULL;   // empty list
const arr_list_t *INVALID_LIST = NULL; // invalid list

int test_compare_node(const void *value_to_find, const void *node_data) {
    int *value_to_find_int = (int *)value_to_find;
    int *node_data_int = (int *)node_data;
    if (*node_data_int > *value_to_find_int) {
        return -1;
    } else if (*node_data_int < *value_to_find_int) {
        return 1;
    } else {
        return 0;
    }
}

int make_mod(void *data, void *addl_data) {
    *((int *)data) = (*(int *)data) % 2;
    (*(size_t *)addl_data)++;
    return SUCCESS;
}

int count_three(void *data, void *addl_data) {
    if (*(size_t *)addl_data < 3) {
        (*(size_t *)addl_data)++;
        return SUCCESS;
    } else {
        return INVALID;
    }
}

int init_suite1(void) { return SUCCESS; }

int clean_suite1(void) { return SUCCESS; }

void test_arr_list_new() {
    // Create empty list
    EMPTY_LIST = arr_list_new(NULL, test_compare_node, 1, sizeof(*data), NULL);
    CU_ASSERT_PTR_NOT_NULL_FATAL(EMPTY_LIST);
    CU_ASSERT_EQUAL(0, arr_list_size(EMPTY_LIST));
    CU_ASSERT_EQUAL(1, arr_list_capacity(EMPTY_LIST));

    // Verify list was created correctly
    list = arr_list_wrap(NULL, test_compare_node, SIZE, sizeof(**int_arr),
                         &wrapped, NULL);
    int_arr = (int **)&wrapped;
    CU_ASSERT_PTR_NOT_NULL_FATAL(list);
    CU_ASSERT_PTR_NOT_NULL_FATAL(*int_arr);
    CU_ASSERT_EQUAL(arr_list_size(list), 0);
    CU_ASSERT_EQUAL(arr_list_capacity(list), SIZE);
}

void test_arr_list_insert() {
    // Should catch if push is called on an invalid list
    CU_ASSERT_NOT_EQUAL(
        SUCCESS, arr_list_insert((arr_list_t *)INVALID_LIST, &data[0], 0));

    CU_ASSERT_PTR_NOT_NULL_FATAL(list);
    for (size_t i = 0; i < SIZE; i++) {
        // Function exited correctly
        CU_ASSERT_EQUAL_FATAL(arr_list_insert(list, &data[i], i), SUCCESS);
        // New node was inserted and points to the correct data
        CU_ASSERT_EQUAL((*int_arr)[i], data[i]);
    }

    // list size is correct
    CU_ASSERT_EQUAL(arr_list_size(list), SIZE);
}

void test_arr_list_get() {
    CU_ASSERT_PTR_NOT_NULL_FATAL(list);
    size_t cur_size = arr_list_size(list);
    size_t position = cur_size / 2;

    // Should catch if get is called on an invalid list
    CU_ASSERT_PTR_NULL(arr_list_get(INVALID_LIST, position));
    // Should catch if get is called on an empty list
    CU_ASSERT_PTR_NULL(arr_list_get(EMPTY_LIST, position));

    int *node = arr_list_get(list, position);
    // Function should have exited successfully
    CU_ASSERT_PTR_NOT_NULL_FATAL(node);
    // Correct value should have been peeked from head node
    CU_ASSERT_EQUAL(data[position], *node);
}

void test_arr_list_sort() {
    // Should catch if sort is called on an invalid list
    CU_ASSERT_NOT_EQUAL(arr_list_sort((arr_list_t *)INVALID_LIST), SUCCESS);

    CU_ASSERT_PTR_NOT_NULL_FATAL(list);
    // Ensure function exited successfully
    CU_ASSERT_EQUAL(arr_list_sort(list), SUCCESS);
    // Verify list should now be sorted in descending order
    int prev = (*int_arr)[0];
    for (size_t i = 1; i < arr_list_size(list); i++) {
        CU_ASSERT_TRUE_FATAL(prev <= (*int_arr)[i]);
    }
}

void test_arr_list_iterator() {
    // Should catch if function is called on an invalid list
    CU_ASSERT_EQUAL(arr_list_iterator_reset((arr_list_t *)INVALID_LIST),
                    EINVAL);
    CU_ASSERT_PTR_NULL(arr_list_iterator_next((arr_list_t *)INVALID_LIST));

    CU_ASSERT_PTR_NOT_NULL_FATAL(list);
    // Confirm iterator is iterating correctly
    CU_ASSERT_EQUAL(arr_list_iterator_reset(list), SUCCESS);
    for (size_t i = 0; i < arr_list_size(list); i++) {
        CU_ASSERT_EQUAL(arr_list_iterator_next(list), &(*int_arr)[i]);
    }
    CU_ASSERT_PTR_NULL(arr_list_iterator_next(list));

    // Confirm iterator is resetting correctly
    CU_ASSERT_EQUAL(arr_list_iterator_reset(list), SUCCESS);
    CU_ASSERT_EQUAL(arr_list_iterator_next(list), &(*int_arr)[0]);
}

void test_arr_list_remove() {
    CU_ASSERT_PTR_NOT_NULL_FATAL(list);
    size_t cur_size = arr_list_size(list);
    int value_to_remove = (*int_arr)[cur_size / 2];

    // Should catch if remove is called on an invalid list
    errno = 0;
    CU_ASSERT_EQUAL(
        arr_list_remove((arr_list_t *)INVALID_LIST, &value_to_remove), INVALID);
    CU_ASSERT_EQUAL(errno, EINVAL);

    // Function should have exited successfully
    CU_ASSERT_EQUAL(arr_list_remove(list, &value_to_remove), true);
    // Size should reflect the removal of the node
    CU_ASSERT_EQUAL_FATAL(arr_list_size(list), cur_size - 1);
    cur_size--;

    // The node containing the removed value should no longer be in the list
    CU_ASSERT_EQUAL(arr_list_remove(list, &value_to_remove), false);
    // Size should not have changed
    CU_ASSERT_EQUAL(arr_list_size(list), cur_size);

    int popped = INVALID;
    arr_list_pop(list, cur_size - 1, &popped);
    CU_ASSERT_EQUAL(popped, (*int_arr)[cur_size - 1]);
    CU_ASSERT_EQUAL_FATAL(arr_list_size(list), cur_size - 1);
}

void test_arr_list_index_of() {
    CU_ASSERT_PTR_NOT_NULL_FATAL(list);
    size_t cur_size = arr_list_size(list);
    size_t position = cur_size / 2;
    int value_to_find = (*int_arr)[position];

    // Should catch if function is called on an invalid list
    CU_ASSERT_EQUAL(arr_list_index_of(INVALID_LIST, &value_to_find, NULL),
                    EINVAL);
    ssize_t idx = 0;
    // Should not be able to find the value in the empty list
    CU_ASSERT_EQUAL(arr_list_index_of(EMPTY_LIST, &value_to_find, &idx),
                    SUCCESS);
    CU_ASSERT_EQUAL(idx, INVALID);

    // Find the first occurrence of the value in the list
    CU_ASSERT_EQUAL(arr_list_index_of(list, &value_to_find, &idx), SUCCESS);
    CU_ASSERT_EQUAL(idx, position);
    CU_ASSERT_EQUAL((*int_arr)[idx], value_to_find);
}

void test_arr_list_set() {
    CU_ASSERT_PTR_NOT_NULL_FATAL(list);
    size_t cur_size = arr_list_size(list);
    size_t position = cur_size / 2;
    int value_to_set = 42;

    // Should catch if set is called on an invalid list
    CU_ASSERT_NOT_EQUAL(
        arr_list_set((arr_list_t *)INVALID_LIST, &value_to_set, position, NULL),
        SUCCESS);

    // Function should have exited successfully
    int old_val_check = (*int_arr)[position];
    int old_value;
    CU_ASSERT_EQUAL(arr_list_set(list, &value_to_set, position, &old_value),
                    SUCCESS);
    // Size should not change
    CU_ASSERT_EQUAL(arr_list_size(list), cur_size);
    // The old value should be the same as the value that was in the list
    CU_ASSERT_EQUAL(old_val_check, old_value);
    // The node containing the set value should now be in the list
    CU_ASSERT_EQUAL((*int_arr)[position], value_to_set);
}

void test_arr_list_foreach() {
    // Should catch if function is called on an invalid list
    CU_ASSERT_EQUAL(
        arr_list_foreach((arr_list_t *)INVALID_LIST, make_mod, NULL), EINVAL);

    CU_ASSERT_PTR_NOT_NULL_FATAL(list);
    size_t iterations = 0;
    CU_ASSERT_EQUAL(arr_list_foreach(list, make_mod, &iterations), SUCCESS);
    // Ensure the user defined action was done to the list nodes
    CU_ASSERT_TRUE((*int_arr)[0] == 0 && (*int_arr)[1] == 1);
    // Ensure the user defined action was done to the correct number of nodes
    CU_ASSERT_EQUAL(iterations, arr_list_size(list));

    iterations = 0;
    CU_ASSERT_EQUAL(arr_list_foreach(list, count_three, &iterations), INVALID);
    CU_ASSERT_EQUAL(iterations, 3);
}

void test_arr_list_clear() {
    // Should catch if clear is called on an invalid list
    CU_ASSERT_NOT_EQUAL(arr_list_clear((arr_list_t *)INVALID_LIST), SUCCESS);

    CU_ASSERT_PTR_NOT_NULL_FATAL(list);
    // Function should have exited successfully
    CU_ASSERT_EQUAL(arr_list_clear(list), SUCCESS);
    // list should now be empty
    CU_ASSERT_EQUAL(arr_list_size(list), 0)
    CU_ASSERT_PTR_NULL(arr_list_get(list, 0));
    CU_ASSERT_EQUAL((*int_arr)[0], 0);
}

void test_arr_list_delete() {
    CU_ASSERT_PTR_NOT_NULL_FATAL(list);
    // Should catch if delete is called on an invalid list
    CU_ASSERT_NOT_EQUAL(arr_list_delete((arr_list_t *)INVALID_LIST), SUCCESS);

    int test_num = 42;
    arr_list_insert(list, &test_num, 0);
    // Function should have exited successfully
    CU_ASSERT_EQUAL(arr_list_delete(list), SUCCESS);
    CU_ASSERT_EQUAL(arr_list_delete((arr_list_t *)EMPTY_LIST), SUCCESS);
    // Destroy should not have freed the data
    CU_ASSERT_EQUAL((*int_arr)[0], test_num);
    free(*int_arr);
}

int main(void) {
    CU_TestInfo suite1_tests[] = {
        {"Testing arr_list_new():", test_arr_list_new},

        {"Testing arr_list_insert():", test_arr_list_insert},

        {"Testing arr_list_get():", test_arr_list_get},

        {"Testing arr_list_sort():", test_arr_list_sort},

        {"Testing arr_list_iterator():", test_arr_list_iterator},

        {"Testing arr_list_remove():", test_arr_list_remove},

        {"Testing arr_list_index_of():", test_arr_list_index_of},

        {"Testing arr_list_set():", test_arr_list_set},

        {"Testing arr_list_foreach():", test_arr_list_foreach},

        {"Testing arr_list_clear():", test_arr_list_clear},

        {"Testing arr_list_delete():", test_arr_list_delete},
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
