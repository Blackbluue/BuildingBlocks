#include "linked_list.h"
#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>
#include <stdio.h>
#include <stdlib.h>

// The integer all node data[5] pointers point to
int data[] = {1, 2, 3, 4, 5};
enum {
    SIZE = sizeof(data) / sizeof(*data),
};
// The list to be used by all the tests
list_t *list = NULL;

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

int make_mod(void **data, void *addl_data) {
    **(int **)data = (*(*(int **)data) % 2);
    return 0;
}

void custom_free(void *mem_addr) {}

int init_suite1(void) { return 0; }

int clean_suite1(void) { return 0; }

void test_list_new() {
    // Verify list was created correctly with no arguments supplied
    list = list_new(NULL, NULL, NULL);
    CU_ASSERT_PTR_NOT_NULL_FATAL(list); // Function exited correctly
    list_delete(&list);
    // NOLINTNEXTLINE

    // Verify list was created correctly with all arguments supplied
    list = list_new(custom_free, test_compare_node, NULL);
    CU_ASSERT_PTR_NOT_NULL_FATAL(list); // Function exited correctly
    // NOLINTNEXTLINE
    CU_ASSERT(0 == list_size(list)); // list size is correct
}

void test_list_push_tail() {
    int exit_code = 1;
    int i = 0;
    list_t *invalid_list = NULL;

    // Should catch if push is called on an invalid list
    CU_ASSERT_FATAL(NULL != list);
    CU_ASSERT(0 != list_push_tail(invalid_list, &data[0]));

    while (i < 5) {
        exit_code = list_push_tail(list, &data[i]);
        if (0 != exit_code) {
            break;
        }
        // New node was pushed and points to the correct data
        CU_ASSERT(data[i] == *(int *)(list_peek_tail(list)));
        i++;
    }

    // Function exited correctly
    CU_ASSERT(0 == exit_code);
    // list size is correct
    // NOLINTNEXTLINE
    CU_ASSERT(5 == list_size(list));
}

// void test_list_is_circular() {
//     CU_ASSERT_FATAL(NULL != list);
//     // NOLINTNEXTLINE
//     CU_ASSERT(list->tail->next == list->head);
// }

void test_list_pop_head() {
    int i = 0;
    list_t *invalid_list = NULL;
    int *node_data = NULL;

    CU_ASSERT_FATAL(NULL != list);
    // Should catch if pop is called on an invalid list
    CU_ASSERT(NULL == list_pop_head(invalid_list));

    // Pop all nodes
    while (i < 5) {
        node_data = list_pop_head(list);
        CU_ASSERT_FATAL(NULL != node_data);
        // NOLINTNEXTLINE
        CU_ASSERT(data[i] == *node_data);
        i++;
    }
    // NOLINTNEXTLINE
    CU_ASSERT(0 == list_size(list));
    // Function should return NULL if pop is called on an empty list
    CU_ASSERT(NULL == list_pop_head(list));
}

void test_list_push_head() {
    int exit_code = 1;
    int i = 0;
    list_t *invalid_list = NULL;

    // Should catch if push is called on an invalid list
    CU_ASSERT_FATAL(NULL != list);
    exit_code = list_push_head(invalid_list, &data[i]);
    CU_ASSERT(0 != exit_code);

    // Push 5 nodes
    while (i < 5) {
        exit_code = list_push_head(list, &data[i]);
        // New node was pushed and points to the correct data
        // NOLINTNEXTLINE
        CU_ASSERT(data[i] == *(int *)(list_peek_head(list)));

        i++;
    }

    // Function exited correctly
    CU_ASSERT(0 == exit_code);
    // list size is correct
    // NOLINTNEXTLINE
    CU_ASSERT(5 == list_size(list));
}

void test_list_sort() {
    int exit_code = 1;
    list_t *invalid_list = NULL;
    int *node_data = NULL;
    int i = 0;

    // Should catch if sort is called on an invalid list
    exit_code = list_sort(invalid_list);
    CU_ASSERT(0 != exit_code);

    exit_code = list_sort(list);
    // Ensure function exited successfully
    CU_ASSERT(0 == exit_code);

    // Verify list should now be reversed
    while (i < 5) {
        node_data = list_pop_head(list);
        CU_ASSERT_FATAL(NULL != node_data);
        // Correct value should be successfully popped from head
        // NOLINTNEXTLINE
        CU_ASSERT(data[i] == *node_data);
        i++;
    }

    // Return the list to its original state for following tests
    i = 0;
    while (i < 5) {
        list_push_head(list, &data[i]);
        i++;
    }
}

void test_list_pop_tail() {
    // int exit_code = 0;
    int i = 0;
    list_t *invalid_list = NULL;
    int *node_data = NULL;

    CU_ASSERT_FATAL(NULL != list);
    // Should catch if pop is called on an invalid list
    node_data = list_pop_tail(invalid_list);
    CU_ASSERT(NULL == node_data);

    // Pop all nodes
    while (i < 5) {
        node_data = list_pop_tail(list);
        // Correct value should be successfully popped into value_from_node
        CU_ASSERT_FATAL(NULL != node_data);
        // NOLINTNEXTLINE
        CU_ASSERT(data[i] == *node_data);
        i++;
    }

    CU_ASSERT(0 == list_size(list));

    // Function should return null if pop is called on an empty list
    node_data = list_pop_head(list);
    CU_ASSERT(NULL == node_data);
}

void test_list_peek_head() {
    list_t *invalid_list = NULL;
    int *node_data = NULL;
    int i = 0;

    CU_ASSERT_FATAL(NULL != list);
    // Should catch if function is called on an invalid list
    node_data = list_peek_head(invalid_list);
    CU_ASSERT(NULL == node_data);

    // Function should not be able to peek on an empty list
    node_data = list_peek_head(list);
    CU_ASSERT(NULL == node_data);

    // Push the nodes tail into list
    while (i < 5) {
        list_push_tail(list, &data[i]);
        i++;
    }

    node_data = list_peek_head(list);

    // Function should have exited successfully
    CU_ASSERT_FATAL(NULL != node_data);
    // Correct value should have been peeked from head node
    CU_ASSERT(data[0] == *node_data);
    // Size shouldn't have changed
    // NOLINTNEXTLINE
    CU_ASSERT(5 == list_size(list));
}

void test_list_iterator() {
    list_t *invalid_list = NULL;

    // Should catch if function is called on an invalid list
    CU_ASSERT(0 != list_iterator_reset(invalid_list));
    CU_ASSERT(NULL == list_iterator_next(invalid_list));

    CU_ASSERT_FATAL(NULL != list);
    // Confirm iterator is iterating correctly
    CU_ASSERT(0 == list_iterator_reset(list));
    for (size_t i = 0; i < list_size(list); i++) {
        CU_ASSERT(&data[i] == list_iterator_next(list));
    }
    CU_ASSERT(NULL == list_iterator_next(list));

    // Confirm iterator is resetting correctly
    CU_ASSERT(0 == list_iterator_reset(list));
    CU_ASSERT(&data[0] == list_iterator_next(list));

    // Size shouldn't have changed
    // NOLINTNEXTLINE
    CU_ASSERT(SIZE == list_size(list));
}

void test_list_remove() {
    int value_to_remove = 4;
    int *removed_value;
    list_t *invalid_list = NULL;

    CU_ASSERT_FATAL(NULL != list);
    // Should catch if remove is called on an invalid list
    removed_value = list_remove(invalid_list, (void *)&value_to_remove, NULL);
    CU_ASSERT(NULL == removed_value);

    removed_value = list_remove(list, (void *)&value_to_remove, NULL);
    // Function should have exited successfully
    CU_ASSERT(value_to_remove == *removed_value);
    // Size should reflect the removal of the node
    // NOLINTNEXTLINE
    CU_ASSERT(4 == list_size(list));

    // The node containing the removed value should no longer be in the list
    removed_value = list_remove(list, (void *)&value_to_remove, NULL);
    CU_ASSERT(NULL == removed_value);
}

void test_list_find_first() {
    int value_to_find = 4;
    list_t *invalid_list = NULL;
    int *node_data = NULL;

    CU_ASSERT_FATAL(NULL != list);
    // Should catch if function is called on an invalid list
    node_data = list_find_first(invalid_list, (void *)&value_to_find);
    CU_ASSERT(NULL == node_data);

    // Should not be able to find the value removed in the previous test
    node_data = list_find_first(list, (void *)&value_to_find);
    CU_ASSERT(NULL == node_data);

    // Change the value we are looking for to one that is still in the list
    value_to_find--;
    node_data = list_find_first(list, (void *)&value_to_find);

    // Ensure function exited successfully
    CU_ASSERT_FATAL(NULL != node_data);
    // Ensure the value found was the one that was searched for
    // NOLINTNEXTLINE
    CU_ASSERT(value_to_find == *node_data);
}

void test_list_foreach_call() {
    int exit_code = 1;
    list_t *invalid_list = NULL;

    CU_ASSERT_FATAL(NULL != list);
    // Should catch if function is called on an invalid list
    exit_code = list_foreach_call(invalid_list, make_mod, NULL);
    CU_ASSERT(0 != exit_code);

    exit_code = list_foreach_call(list, make_mod, NULL);

    // Ensure function exited correctly
    CU_ASSERT(0 == exit_code);

    // Ensure the user defined action was done to the list nodes
    // NOLINTNEXTLINE
    CU_ASSERT(1 == *(int *)list_get(list, 0) && 0 == *(int *)list_get(list, 1));
}

void test_list_find_all() {
    int value_to_find = 1;
    list_t *test_list = NULL;
    list_t *result_list = NULL;
    unsigned int i = 0;

    CU_ASSERT_FATAL(NULL != list);
    // Should catch if function is called on an invalid list
    result_list = list_find_all(test_list, (void *)&value_to_find);
    CU_ASSERT(NULL == result_list);

    // create empty list
    test_list = list_new((FREE_F)custom_free, (CMP_F)test_compare_node, NULL);

    // Should catch if function is called on an empty list
    result_list = list_find_all(test_list, (void *)&value_to_find);
    CU_ASSERT(NULL == result_list);

    result_list = list_find_all(list, (void *)&value_to_find);

    // Ensure function exited successfully
    CU_ASSERT_FATAL(NULL != result_list);
    // Verify that each node of the resulting list is the correct value
    // NOLINTNEXTLINE
    while (i < list_size(result_list)) {
        CU_ASSERT(value_to_find == *(int *)list_get(result_list, i));
        i++;
    }

    // Verify that the correct number of occurrences were found
    // NOLINTNEXTLINE
    CU_ASSERT(3 == list_size(result_list));

    list_delete(&test_list);
    list_delete(&result_list);
}

void test_list_get() {
    size_t position = 2;
    list_t *invalid_list = NULL;
    int *node = NULL;

    CU_ASSERT_FATAL(NULL != list);
    // Should catch if get is called on an invalid list
    node = list_get(invalid_list, position);
    CU_ASSERT(NULL == node);

    // Should catch if get is called on an empty list
    invalid_list =
        list_new((FREE_F)custom_free, (CMP_F)test_compare_node, NULL);
    CU_ASSERT_FATAL(NULL != invalid_list);
    node = list_get(invalid_list, position);
    CU_ASSERT(NULL == node);
    list_delete(&invalid_list);

    node = list_get(list, position);

    // Function should have exited successfully
    CU_ASSERT_FATAL(NULL != node);
    // Correct value should have been peeked from head node
    // NOLINTNEXTLINE
    CU_ASSERT(data[position] == *node);
    // Size shouldn't have changed
    CU_ASSERT(4 == list_size(list));
}

void test_list_insert() {
    int exit_code = 1;
    int value_to_insert = 9;
    size_t position = 2;
    list_t *invalid_list = NULL;

    CU_ASSERT_FATAL(NULL != list);
    // Should catch if insert is called on an invalid list
    exit_code = list_insert(invalid_list, (void *)&value_to_insert, position);
    CU_ASSERT(0 != exit_code);

    exit_code = list_insert(list, (void *)&value_to_insert, position);
    // Function should have exited successfully
    CU_ASSERT(0 == exit_code);
    // Size should reflect the insertion of the node
    // NOLINTNEXTLINE
    CU_ASSERT(5 == list_size(list));

    // The node containing the inserted value should now be in the list
    // NOLINTNEXTLINE
    int *node_data = list_get(list, position);
    CU_ASSERT(value_to_insert == *node_data);
}

void test_list_clear() {
    int exit_code = 1;
    list_t *invalid_list = NULL;

    CU_ASSERT_FATAL(NULL != list);
    // Should catch if clear is called on an invalid list
    exit_code = list_clear(invalid_list);
    CU_ASSERT(0 != exit_code);

    exit_code = list_clear(list);
    // list should now be empty
    // NOLINTNEXTLINE
    CU_ASSERT(0 == list_size(list))
    // NOLINTNEXTLINE
    CU_ASSERT(NULL == list_peek_head(list));

    // Function should have exited successfully
    CU_ASSERT(0 == exit_code);
}

void test_list_peek_tail() {
    list_t *invalid_list = NULL;
    int *node_data = NULL;
    int i = 0;

    CU_ASSERT_FATAL(NULL != list);
    // Should catch if pop is called on an invalid list
    node_data = list_peek_tail(invalid_list);
    CU_ASSERT(NULL == node_data);

    // Function should not be able to peek on an empty list
    node_data = list_peek_tail(list);
    CU_ASSERT(NULL == node_data);

    // Push the nodes tail into list
    while (i < 5) {
        list_push_tail(list, &data[i]);
        i++;
    }

    node_data = list_peek_tail(list);

    // Function should have exited successfully
    CU_ASSERT_FATAL(NULL != node_data);
    // Correct value should have been peeked from head node
    // NOLINTNEXTLINE
    CU_ASSERT(data[4] == *node_data);
    // Size shouldn't have changed
    CU_ASSERT(5 == list_size(list));
}

void test_list_delete() {
    int exit_code = 1;
    list_t *invalid_list = NULL;

    CU_ASSERT_FATAL(NULL != list);
    // Should catch if delete is called on an invalid list
    exit_code = list_delete(&invalid_list);
    CU_ASSERT(0 != exit_code);

    // Function should have exited successfully
    exit_code = list_delete(&list);
    CU_ASSERT(0 == exit_code);

    // Should catch if delete is called on the list that has already been
    // deleted
    exit_code = list_delete(&list);
    CU_ASSERT(0 != exit_code);
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
