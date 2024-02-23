#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>
#include <hash_table.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SUCCESS 0
hash_table_t *hash_table = NULL;
int data[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

int init_suite1(void) { return 0; }

int clean_suite1(void) { return 0; }

/**
 * @brief Test free, does nothing
 *
 * @param mem_addr Not used
 */
void custom_free(void *mem_addr) {}

void test_hash_table_init() {
    size_t capacity = sizeof(data) / sizeof(data[0]);
    // Verify hash_table was created correctly
    hash_table = hash_table_init(capacity, custom_free, (CMP_F)strcmp);
}

void test_hash_table_set() {
    hash_table_t *invalid_table = NULL;

    // Should catch if add is called on an invalid pointer
    CU_ASSERT(EINVAL ==
              hash_table_set(invalid_table, (void *)&data[3], "Invalid"));

    // Should all work
    CU_ASSERT(SUCCESS ==
              hash_table_set(hash_table, (void *)&data[0], "Item one"));
    CU_ASSERT(SUCCESS ==
              hash_table_set(hash_table, (void *)&data[1], "Item two"));
    CU_ASSERT(SUCCESS ==
              hash_table_set(hash_table, (void *)&data[2], "Item three"));
    CU_ASSERT(SUCCESS ==
              hash_table_set(hash_table, (void *)&data[3], "Item four"));
    CU_ASSERT(SUCCESS ==
              hash_table_set(hash_table, (void *)&data[4], "Item five"));
    CU_ASSERT(SUCCESS ==
              hash_table_set(hash_table, (void *)&data[5], "Item six"));
    CU_ASSERT(SUCCESS ==
              hash_table_set(hash_table, (void *)&data[6], "Item seven"));
    CU_ASSERT(SUCCESS ==
              hash_table_set(hash_table, (void *)&data[7], "Item eight"));
    CU_ASSERT(SUCCESS ==
              hash_table_set(hash_table, (void *)&data[8], "Item nine"));
    CU_ASSERT(SUCCESS ==
              hash_table_set(hash_table, (void *)&data[9], "Item ten"));
}

void test_hash_table_lookup() {
    hash_table_t *invalid_table = NULL;

    // Should catch if create is called on an invalid pointer
    void *return_ptr = hash_table_lookup(invalid_table, "Item three");
    CU_ASSERT(NULL == return_ptr);

    // ensure unique nodes are created per key value
    // each node is created with data fields of the same value
    CU_ASSERT(SUCCESS == hash_table_set(hash_table, (void *)&data[0], "key1"));
    CU_ASSERT(SUCCESS == hash_table_set(hash_table, (void *)&data[0], "key2"));
    return_ptr = hash_table_lookup(hash_table, "key1");
    CU_ASSERT_FATAL(NULL != return_ptr);
    CU_ASSERT_FATAL(return_ptr == hash_table_lookup(hash_table, "key2"));

    // check normal returns
    return_ptr = hash_table_lookup(hash_table, "Item two");
    CU_ASSERT_FATAL((void *)&data[1] == return_ptr);

    return_ptr = hash_table_lookup(hash_table, "Item three");
    CU_ASSERT_FATAL((void *)&data[2] == return_ptr);
}

void test_hash_table_remove() {
    hash_table_t *invalid_table = NULL;

    // Should catch if create is called on an invalid pointer
    void *value = hash_table_remove(invalid_table, "Item three");
    CU_ASSERT(NULL == value);

    void *ret_ptr = hash_table_lookup(hash_table, "Item three");
    CU_ASSERT((void *)&data[2] == ret_ptr);

    value = hash_table_remove(hash_table, "Item three");
    CU_ASSERT((void *)&data[2] == value);

    ret_ptr = hash_table_lookup(hash_table, "Item three");
    CU_ASSERT_FATAL(NULL == ret_ptr);

    value = hash_table_remove(hash_table, "Item three");
    CU_ASSERT(NULL == value);
}

void test_hash_table_clear() {
    hash_table_t *invalid_table = NULL;

    int exit_code = hash_table_clear(invalid_table);
    CU_ASSERT(EINVAL == exit_code);

    exit_code = hash_table_clear(hash_table);
    CU_ASSERT(SUCCESS == exit_code);
}

void test_hash_table_destroy() {
    hash_table_t *invalid_table = NULL;

    int exit_code = hash_table_destroy(&invalid_table);
    CU_ASSERT(EINVAL == exit_code);

    exit_code = hash_table_destroy(&hash_table);
    CU_ASSERT(SUCCESS == exit_code);

    exit_code = hash_table_destroy(&hash_table);
    CU_ASSERT(EINVAL == exit_code);
}

int main(void) {
    CU_TestInfo suite1_tests[] = {
        {"Testing hash_table_init():", test_hash_table_init},

        {"Testing hash_table_set():", test_hash_table_set},

        {"Testing hash_table_lookup():", test_hash_table_lookup},

        {"Testing hash_table_remove():", test_hash_table_remove},

        {"Testing hash_table_clear():", test_hash_table_clear},

        {"Testing hash_table_destroy():", test_hash_table_destroy},

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
    puts("\n");
    return num_failed;
}
