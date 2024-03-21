#include "stack.h"
#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>
#include <stdlib.h>

#define CAPACITY 5

// The stack to be used by all the tests
stack_t *stack = NULL;
// The integer all node data[5] pointers point to
int data[] = {1, 2, 3, 4, 5};

int init_suite1(void) { return 0; }

int clean_suite1(void) { return 0; }

/**
 * @brief frees an item and its associated memory
 *
 * @param mem_addr pointer of the item to be free'd
 */
void custom_free(void *mem_addr) { (void)mem_addr; }

void test_stack_init() {
    // Verify stack was created correctly
    stack = stack_init(CAPACITY, custom_free);
    CU_ASSERT_FATAL(NULL != stack);
    // NOLINTNEXTLINE
    CU_ASSERT(CAPACITY == stack_get_capacity(stack));
    // NOLINTNEXTLINE
    CU_ASSERT(0 == stack_get_size(stack));
}

void test_stack_push() {
    stack_t *invalid_stack = NULL;

    CU_ASSERT_FATAL(NULL != stack);
    // Should catch if push is called on an invalid stack or with invalid data
    int exit_code = stack_push(invalid_stack, &data[0]);
    CU_ASSERT(0 != exit_code);

    // push CAPACITY number of nodes
    for (size_t i = 0; i < CAPACITY; i++) {
        exit_code = stack_push(stack, &data[i]);
        // New node was pushed and points to the correct data
        CU_ASSERT(&data[i] == stack_peek(stack));
        // Function exited correctly
        CU_ASSERT(0 == exit_code);
    }

    // stack size is correct
    CU_ASSERT(CAPACITY == stack_get_size(stack));

    // Function should return a code if push is called on a full stack
    exit_code = stack_push(stack, &data[5]);
    CU_ASSERT(0 != exit_code);
}

void test_stack_pop() {
    stack_t *invalid_stack = NULL;

    CU_ASSERT_FATAL(NULL != stack);
    // Should catch if push is called on an invalid stack
    void *value = stack_pop(invalid_stack);
    CU_ASSERT(NULL == value);

    // pop all items
    for (size_t i = 0; i < CAPACITY; i++) {
        value = stack_pop(stack);
        CU_ASSERT_FATAL(NULL != value);
        // NOLINTNEXTLINE
        CU_ASSERT(&data[stack_get_size(stack)] == value);
        value = NULL;
    }

    // Should return NULL when called on empty stack
    value = stack_pop(stack);
    CU_ASSERT(NULL == value);
    CU_ASSERT(0 == stack_get_size(stack));
}

void test_stack_peek() {
    stack_t *invalid_stack = NULL;

    // Should catch if pop is called on an invalid stack or empty
    void *value = stack_peek(invalid_stack);
    CU_ASSERT(NULL == value);
    value = stack_peek(stack);
    CU_ASSERT(NULL == value);

    // push CAPACITY number of nodes
    for (size_t i = 0; i < CAPACITY; i++) {
        stack_push(stack, &data[i]);
    }

    value = stack_peek(stack);

    // Function should have exited successfully
    CU_ASSERT_FATAL(NULL != value);
    // Correct value should have been peeked from front node
    CU_ASSERT(data[CAPACITY - 1] == *(int *)value);
    // Size shouldn't have changed
    CU_ASSERT(CAPACITY == stack_get_size(stack));
}

void test_stack_clear() {
    stack_t *invalid_stack = NULL;

    CU_ASSERT_FATAL(NULL != stack);
    // Should catch if clear is called on an invalid stack
    int exit_code = stack_clear(invalid_stack);
    CU_ASSERT(0 != exit_code);

    exit_code = stack_clear(stack);
    // stack should now be empty
    CU_ASSERT(0 == stack_get_size(stack));
    // Function should have exited fully
    CU_ASSERT(0 == exit_code);
}

void test_stack_destroy() {
    stack_t *invalid_stack = NULL;

    // Should catch if delete is called on an invalid stack
    int exit_code = stack_destroy(&invalid_stack);
    CU_ASSERT(0 != exit_code);

    // Function should have exited fully
    exit_code = stack_destroy(&stack);
    CU_ASSERT(0 == exit_code);

    // Should catch if delete is called on the stack that has already been
    // deleted
    exit_code = stack_destroy(&stack);
    CU_ASSERT(0 != exit_code);
}

int main(void) {
    CU_TestInfo suite1_tests[] = {
        {"Testing stack_init():", test_stack_init},

        {"Testing stack_push():", test_stack_push},

        {"Testing stack_pop():", test_stack_pop},

        {"Testing stack_peek():", test_stack_peek},

        {"Testing stack_clear():", test_stack_clear},

        {"Testing stack_destroy():", test_stack_destroy}, CU_TEST_INFO_NULL};

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
