#include "avl_tree.h"
#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

enum {
    CAPACITY = 10,
    SUCCESS = 0,
    INVALID = -1,
    FOUND = 42,
};
// The tree to be used by all the tests
tree_t *tree = NULL;
// The integer all node data[] pointers point to
int data[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
int even_odd[CAPACITY];

int init_suite1(void) { return 0; }

int clean_suite1(void) { return 0; }

void custom_free(void *data) {}

int test_compare_node(const void *value_to_find, const void *node_data) {
    if (*(int *)node_data > *(int *)value_to_find) {
        return -1;
    } else if (*(int *)node_data < *(int *)value_to_find) {
        return 1;
    } else {
        return 0;
    }
}

int sum_up(void *node_data, void *addl_data) {
    int *sum = addl_data;
    *sum += *(int *)node_data;
    return 0;
}

int find(void *node_data, void *addl_data) {
    int *found = addl_data;
    if (*(int *)node_data == *found) {
        return FOUND;
    }
    return 0;
}

void test_tree_new() {
    // Verify tree was not created correctly with no arguments supplied
    errno = 0;
    tree = tree_new(NULL, NULL);
    CU_ASSERT(tree == NULL);    // Function exited correctly
    CU_ASSERT(errno == EINVAL); // errno is correct

    // Verify tree was created correctly with all arguments supplied
    tree = tree_new(custom_free, test_compare_node);
    CU_ASSERT_FATAL(tree != NULL);           // Function exited correctly
    CU_ASSERT(tree_size(tree) == 0);         // tree size is correct
    CU_ASSERT(tree_is_empty(tree) != false); // tree is empty
}

void test_tree_add() {
    // Verify tree_add() fails with NULL tree
    CU_ASSERT(tree_add(NULL, NULL) == EINVAL); // Function exited correctly

    CU_ASSERT_FATAL(tree != NULL); // tree is not NULL
    // Verify tree_add() succeeds with valid data and tree
    for (size_t i = 0; i < CAPACITY; i++) {
        CU_ASSERT(tree_add(tree, &data[i]) == SUCCESS);
        CU_ASSERT(tree_size(tree) == i + 1); // tree size is correct
    }
    CU_ASSERT(tree_is_empty(tree) == false); // tree is not empty
}

void test_tree_contains() {
    // Verify tree_contains() fails with NULL tree
    errno = 0;
    CU_ASSERT(tree_contains(NULL, NULL) ==
              INVALID);         // Function exited correctly
    CU_ASSERT(errno == EINVAL); // errno is correct

    CU_ASSERT_FATAL(tree != NULL); // tree is not NULL
    // Verify tree_contains() succeeds with valid data and tree
    for (size_t i = 0; i < CAPACITY; i++) {
        CU_ASSERT(tree_contains(tree, &data[i]) != false);
    }
    int not_in_tree = 100;
    CU_ASSERT(tree_contains(tree, &not_in_tree) == false);
}

void test_tree_find_first() {
    // Verify tree_find_first() fails with NULL tree
    errno = 0;
    CU_ASSERT(tree_find_first(NULL, NULL) == NULL); // Function exited correctly
    CU_ASSERT(errno == EINVAL);                     // errno is correct

    CU_ASSERT_FATAL(tree != NULL); // tree is not NULL
    // Verify tree_find_first() succeeds with valid data and tree
    for (size_t i = 0; i < CAPACITY; i++) {
        CU_ASSERT(tree_find_first(tree, &data[i]) != NULL);
    }
    CU_ASSERT(tree_size(tree) == CAPACITY); // tree size is correct
    int not_in_tree = 100;
    CU_ASSERT(tree_find_first(tree, &not_in_tree) == NULL);
}

void test_tree_foreach() {
    // Verify tree_foreach() fails with NULL tree
    errno = 0;
    CU_ASSERT(tree_foreach(NULL, NULL, NULL) ==
              INVALID);         // Function exited correctly
    CU_ASSERT(errno == EINVAL); // errno is correct

    CU_ASSERT_FATAL(tree != NULL); // tree is not NULL
    // Verify tree_foreach() succeeds with valid data and tree
    int sum = 0;
    CU_ASSERT(tree_foreach(tree, sum_up, &sum) == SUCCESS);
    CU_ASSERT(sum == 45); // sum is correct

    int found = 7;
    CU_ASSERT(tree_foreach(tree, find, &found) == FOUND);
    int not_found = 11;
    CU_ASSERT(tree_foreach(tree, find, &not_found) != FOUND);
}

void test_tree_iterate() {
    // Verify tree_iterator_reset() fails with NULL tree
    CU_ASSERT(tree_iterator_reset(NULL) == EINVAL); // Function exited correctly

    CU_ASSERT_FATAL(tree != NULL); // tree is not NULL
    // Verify tree_iterator_next() fails if iterator is not reset
    CU_ASSERT(tree_iterator_next(tree) == NULL);
    // Verify tree_iterator_reset() succeeds with valid data and tree
    CU_ASSERT(tree_iterator_reset(tree) == SUCCESS);
    // Verify tree_iterator_next() succeeds with valid data and tree
    size_t even = 0;
    size_t odd = 0;
    for (size_t i = 0; i < CAPACITY; i++) {
        int *value = tree_iterator_next(tree);
        CU_ASSERT_FATAL(value != NULL);
        if (*value % 2 == 0) {
            even++;
        } else {
            odd++;
        }
    }
    CU_ASSERT(even == 5); // even count is correct
    CU_ASSERT(odd == 5);  // even count is correct
    CU_ASSERT(tree_iterator_next(tree) == NULL);
}

void test_tree_remove() {
    // Verify tree_remove() fails with NULL tree
    errno = 0;
    CU_ASSERT(tree_remove(NULL, NULL) == NULL); // Function exited correctly
    CU_ASSERT(errno == EINVAL);                 // errno is correct

    CU_ASSERT_FATAL(tree != NULL); // tree is not NULL
    // Verify tree_remove() succeeds with valid data and tree
    for (size_t i = 0; i < CAPACITY; i++) {
        CU_ASSERT(tree_remove(tree, &data[i]) == &data[i]);
        CU_ASSERT(tree_size(tree) == CAPACITY - i - 1); // tree size is correct
    }
    CU_ASSERT(tree_is_empty(tree) != false); // tree is empty
}

void test_tree_delete() {
    // Verify tree_delete() fails with NULL tree
    CU_ASSERT(tree_delete(NULL) == EINVAL); // Function exited correctly

    CU_ASSERT_FATAL(tree != NULL); // tree is not NULL
    // Verify tree_delete() succeeds with valid data and tree
    CU_ASSERT(tree_delete(&tree) == SUCCESS);
    CU_ASSERT(tree == NULL); // tree is NULL
}

void test_tree_find_all() {
    // Verify tree_find_all() fails with NULL tree
    errno = 0;
    CU_ASSERT(tree_find_all(NULL, NULL) == NULL); // Function exited correctly
    CU_ASSERT(errno == EINVAL);                   // errno is correct

    // init odd/even tree
    tree = tree_new(custom_free, test_compare_node);
    CU_ASSERT_FATAL(tree != NULL); // tree is not NULL
    for (size_t i = 0; i < CAPACITY; i++) {
        even_odd[i] = i % 2;
        tree_add(tree, &even_odd[i]);
    }

    // Verify tree_find_all() succeeds with valid data and tree
    int even = 0;
    tree_t *result = tree_find_all(tree, &even);
    CU_ASSERT_FATAL(result != NULL);
    CU_ASSERT(tree_size(result) == (CAPACITY / 2)); // result size is correct
    tree_delete(&result);

    int not_in_tree = 100;
    result = tree_find_all(tree, &not_in_tree);
    CU_ASSERT_FATAL(result != NULL);
    CU_ASSERT(tree_size(result) == 0); // result size is correct
    tree_delete(&result);
}

void test_tree_remove_all() {
    // Verify tree_remove_all() fails with NULL tree
    errno = 0;
    CU_ASSERT(tree_remove_all(NULL, NULL) ==
              INVALID);         // Function exited correctly
    CU_ASSERT(errno == EINVAL); // errno is correct

    CU_ASSERT_FATAL(tree != NULL); // tree is not NULL
    // Verify tree_remove_all() succeeds with valid data and tree
    int even = 0;
    size_t even_cnt = (CAPACITY / 2);
    CU_ASSERT(tree_remove_all(tree, &even) == even_cnt);
    CU_ASSERT(tree_size(tree) == CAPACITY - even_cnt); // tree size is correct
}

void test_tree_clear() {
    // Verify tree_clear() fails with NULL tree
    CU_ASSERT(tree_clear(NULL) == EINVAL); // Function exited correctly

    CU_ASSERT_FATAL(tree != NULL); // tree is not NULL
    // Verify tree_clear() succeeds with valid data and tree
    CU_ASSERT(tree_clear(tree) == SUCCESS);
    CU_ASSERT(tree_size(tree) == 0);         // tree size is correct
    CU_ASSERT(tree_is_empty(tree) != false); // tree is empty
    tree_delete(&tree);
}

int main(void) {
    CU_TestInfo suite1_tests[] = {
        {"Testing tree_new():", test_tree_new},

        {"Testing tree_add():", test_tree_add},

        {"Testing tree_contains():", test_tree_contains},

        {"Testing tree_find_first():", test_tree_find_first},

        {"Testing tree_foreach():", test_tree_foreach},

        {"Testing tree_iterate:", test_tree_iterate},

        {"Testing tree_remove():", test_tree_remove},

        {"Testing tree_delete():", test_tree_delete},

        {"Testing tree_find_all():", test_tree_find_all},

        {"Testing tree_remove_all():", test_tree_remove_all},

        {"Testing tree_clear():", test_tree_clear},

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
