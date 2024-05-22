#define _POSIX_C_SOURCE 200112L
#include "secure_utils.h"
#include "serialization.h"
#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>
#include <netdb.h>

#define SUCCESS 0
#define FAILURE -1
#define TIMEOUT TIMEOUT_DEFAULT * 5

io_info_t *server_io;
ssl_loader_t *loader = NULL;

int init_suite1(void) {
    if (!SSL_AVAILABLE) {
        fprintf(stderr, "SSL not available\n");
        return FAILURE;
    }
    int err;
    int err_type;
    server_io = new_connect_io_info("", TCP_PORT, &err, &err_type);
    if (server_io == NULL) {
        switch (err_type) {
        case SYS:
            fprintf(stderr, "open_inet_socket: %s\n", strerror(err));
            break;
        case GAI:
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
            break;
        case SOCK:
            fprintf(stderr, "socket: %s\n", strerror(err));
            break;
        case CONN:
            fprintf(stderr, "connect: %s\n", strerror(err));
            break;
        case LISTEN:
            fprintf(stderr, "listen: %s\n", strerror(err));
            break;
        }
        return FAILURE;
    }
    return SUCCESS;
}

int clean_suite1(void) {
    free_io_info(server_io);
    free_ssl_loader(loader);
    return SUCCESS;
}

void test_send_message() {
    CU_ASSERT_PTR_NOT_NULL_FATAL(loader = new_ssl_loader(NULL));
    CU_ASSERT_EQUAL_FATAL(io_info_add_ssl(server_io, loader), SUCCESS);

    char *msg = "hello world!";
    CU_ASSERT_EQUAL_FATAL(
        write_pkt_data(server_io, msg, strlen(msg) + 1, RQU_MSG), SUCCESS);

    struct packet *pkt = recv_pkt_data(server_io, TIMEOUT, NULL);
    CU_ASSERT_PTR_NOT_NULL_FATAL(pkt);
    CU_ASSERT_EQUAL(pkt->hdr->data_type, SVR_SUCCESS);
    CU_ASSERT_EQUAL(pkt->hdr->data_len, strlen(msg) + 1);
    CU_ASSERT_STRING_EQUAL(pkt->data, msg);
    free_packet(pkt);
}

int main(void) {
    allow_graceful_exit();
    CU_TestInfo suite1_tests[] = {
        {"Testing test_send_message():", test_send_message},
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
