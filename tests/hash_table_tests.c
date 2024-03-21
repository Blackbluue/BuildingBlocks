#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>
#include <hash_table.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SUCCESS 0
#define INVALID_TABLE NULL

hash_table_t *hash_table = NULL;
int data[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

int init_suite1(void) { return 0; }

int clean_suite1(void) { return 0; }

/**
 * @brief Test free, does nothing
 *
 * @param mem_addr Not used
 */
void custom_free(void *mem_addr) { (void)mem_addr; }

void test_hash_table_init() {
    size_t capacity = sizeof(data) / sizeof(data[0]);
    // Verify hash_table was created correctly
    int err = SUCCESS;
    CU_ASSERT_PTR_NULL(hash_table_init(capacity, NULL, NULL, &err));
    CU_ASSERT_EQUAL(err, EINVAL);

    hash_table = hash_table_init(capacity, custom_free, (CMP_F)strcmp, NULL);
    CU_ASSERT_PTR_NOT_NULL_FATAL(hash_table);
}

void test_hash_table_set() {
    // Should catch if add is called on an invalid pointer
    CU_ASSERT_EQUAL(hash_table_set(INVALID_TABLE, &data[3], "Invalid"), EINVAL);

    // Should all work
    CU_ASSERT_EQUAL(hash_table_set(hash_table, &data[0], "Item one"), SUCCESS);
    CU_ASSERT_EQUAL(hash_table_set(hash_table, &data[1], "Item two"), SUCCESS);
    CU_ASSERT_EQUAL(hash_table_set(hash_table, &data[2], "Item three"),
                    SUCCESS);
    CU_ASSERT_EQUAL(hash_table_set(hash_table, &data[3], "Item four"), SUCCESS);
    CU_ASSERT_EQUAL(hash_table_set(hash_table, &data[4], "Item five"), SUCCESS);
    CU_ASSERT_EQUAL(hash_table_set(hash_table, &data[5], "Item six"), SUCCESS);
    CU_ASSERT_EQUAL(hash_table_set(hash_table, &data[6], "Item seven"),
                    SUCCESS);
    CU_ASSERT_EQUAL(hash_table_set(hash_table, &data[7], "Item eight"),
                    SUCCESS);
    CU_ASSERT_EQUAL(hash_table_set(hash_table, &data[8], "Item nine"), SUCCESS);
    CU_ASSERT_EQUAL(hash_table_set(hash_table, &data[9], "Item ten"), SUCCESS);
}

void test_hash_table_lookup() {
    // Should catch if create is called on an invalid pointer
    CU_ASSERT_PTR_NULL(hash_table_lookup(INVALID_TABLE, "Item three"));

    // ensure unique nodes are created per key value
    // each node is created with data fields of the same value
    CU_ASSERT_EQUAL(hash_table_set(hash_table, &data[0], "key1"), SUCCESS);
    CU_ASSERT_EQUAL(hash_table_set(hash_table, &data[0], "key2"), SUCCESS);
    void *return_ptr = hash_table_lookup(hash_table, "key1");
    CU_ASSERT_PTR_NOT_NULL_FATAL(return_ptr);
    CU_ASSERT_PTR_EQUAL(return_ptr, hash_table_lookup(hash_table, "key2"));

    // check normal returns
    CU_ASSERT_PTR_EQUAL_FATAL(hash_table_lookup(hash_table, "Item two"),
                              &data[1]);
    CU_ASSERT_PTR_EQUAL_FATAL(hash_table_lookup(hash_table, "Item three"),
                              &data[2]);
}

void test_hash_table_remove() {
    // Should catch if create is called on an invalid pointer
    CU_ASSERT_PTR_NULL(hash_table_remove(INVALID_TABLE, "Item three"));
    CU_ASSERT_PTR_EQUAL(hash_table_lookup(hash_table, "Item three"), &data[2]);
    CU_ASSERT_PTR_EQUAL(hash_table_remove(hash_table, "Item three"), &data[2]);
    CU_ASSERT_PTR_NULL_FATAL(hash_table_lookup(hash_table, "Item three"));
    CU_ASSERT_PTR_NULL(hash_table_remove(hash_table, "Item three"));
}

void test_hash_table_clear() {
    CU_ASSERT_EQUAL(hash_table_clear(INVALID_TABLE), EINVAL);
    CU_ASSERT_EQUAL(hash_table_clear(hash_table), SUCCESS);
}

void test_hash_table_destroy() {
    CU_ASSERT_EQUAL(hash_table_destroy(INVALID_TABLE), EINVAL);
    CU_ASSERT_EQUAL(hash_table_destroy(&hash_table), SUCCESS);
    CU_ASSERT_EQUAL(hash_table_destroy(&hash_table), EINVAL);
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
