#include "networking_client.h"
#include "utils.h"
#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>

#define SUCCESS 0
#define FAILURE -1

int init_suite1(void) { return SUCCESS; }

int clean_suite1(void) { return SUCCESS; }

int main(void) {
    CU_TestInfo suite1_tests[] = {
        // {"Testing test_recv_hero():", test_recv_hero},
        CU_TEST_INFO_NULL,
    };
    CU_SuiteInfo suites[] = {
        {"Suite-1:", init_suite1, clean_suite1, .pTests = suite1_tests},
        CU_SUITE_INFO_NULL,
    };

    if (CU_initialize_registry() != CUE_SUCCESS) {
        return CU_get_error();
    }

    if (CU_register_suites(suites) != CUE_SUCCESS) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_ErrorCode err = CU_basic_run_tests();
    if (err != CUE_SUCCESS) {
        CU_cleanup_registry();
        return err;
    }
    CU_basic_show_failures(CU_get_failure_list());
    int num_failed = CU_get_number_of_failures();
    CU_cleanup_registry();
    puts("\n");
    return num_failed;
}
