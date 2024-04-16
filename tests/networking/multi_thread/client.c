#define _POSIX_C_SOURCE 200112L
#include "networking_client.h"
#include "utils.h"
#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

#define SUCCESS 0
#define FAILURE -1
#define TIMEOUT TO_DEFAULT * 5
#define TEST_COUNT 5

#define NUM(VAL) #VAL
#define STR(VAL) NUM(VAL)

static void exit_handler(int sig) { (void)sig; }

void allow_graceful_exit(void) {
    struct sigaction action;
    action.sa_handler = exit_handler;
    action.sa_flags = 0;
    sigemptyset(&action.sa_mask);
    sigaction(SIGPIPE, &action, NULL);
}

int init_suite1(void) { return SUCCESS; }

int clean_suite1(void) { return SUCCESS; }

long test_ltr_count(int server_sock) {
    // TODO: add randomness to test
    // send a message to the server
    uint16_t count = 2;
    char *string = "hello";
    struct counter_packet request;
    snprintf(request.string, sizeof(request.string), "%s", string);

    long thread_err =
        write_pkt_data(server_sock, &request, sizeof(request), RQU_COUNT);
    if (thread_err != SUCCESS) {
        fprintf(stderr, "write_pkt_data: %s\n", strerror(thread_err));
        return thread_err;
    }

    // get a response from the server
    int err;
    struct packet *pkt = recv_pkt_data(server_sock, TIMEOUT, &err);
    if (pkt == NULL) {
        fprintf(stderr, "recv_pkt_data: %s\n", strerror(err));
        thread_err = err;
        goto cleanup;
    }

    // test the response
    if (pkt->hdr->data_type != SVR_SUCCESS &&
        pkt->hdr->data_type != SVR_FAILURE) {
        fprintf(stderr, "recv_pkt_data: unexpected data type\n");
        thread_err = FAILURE;
        goto cleanup;
    }
    if (pkt->data == NULL ||
        pkt->hdr->data_len != sizeof(struct counter_packet)) {
        fprintf(stderr, "recv_pkt_data: unexpected data\n");
        thread_err = FAILURE;
        goto cleanup;
    }
    struct counter_packet *response = pkt->data;
    char expected_response = 'l'; // TODO: set this to the expected response
    if (response->count != count || response->character != expected_response) {
        fprintf(stderr, "recv_pkt_data: unexpected data values\n");
        thread_err = FAILURE;
        goto cleanup;
    }

cleanup:
    free_packet(pkt);
    return thread_err;
}

long test_repeat(int server_sock) {
    // TODO: add randomness to test
    // send a message to the server
    uint16_t count = 5;
    char letter = 'a';
    struct counter_packet request = {
        .count = count,
        .character = letter,
    };

    long thread_err =
        write_pkt_data(server_sock, &request, sizeof(request), RQU_REPEAT);
    if (thread_err != SUCCESS) {
        fprintf(stderr, "write_pkt_data: %s\n", strerror(thread_err));
        return thread_err;
    }

    // get a response from the server
    int err;
    struct packet *pkt = recv_pkt_data(server_sock, TIMEOUT, &err);
    if (pkt == NULL) {
        fprintf(stderr, "recv_pkt_data: %s\n", strerror(err));
        thread_err = err;
        goto cleanup;
    }

    // test the response
    if (pkt->hdr->data_type != SVR_SUCCESS &&
        pkt->hdr->data_type != SVR_FAILURE) {
        fprintf(stderr, "recv_pkt_data: unexpected data type\n");
        thread_err = FAILURE;
        goto cleanup;
    }
    if (pkt->data == NULL ||
        pkt->hdr->data_len != sizeof(struct counter_packet)) {
        fprintf(stderr, "recv_pkt_data: unexpected data\n");
        thread_err = FAILURE;
        goto cleanup;
    }
    struct counter_packet *response = pkt->data;
    char *expected_response = ""; // TODO: set this to the expected response
    int cmp_res =
        strncmp(response->string, expected_response, strlen(response->string));
    if (response->count != count || cmp_res != SUCCESS) {
        fprintf(stderr, "recv_pkt_data: unexpected data values\n");
        thread_err = FAILURE;
        goto cleanup;
    }

cleanup:
    free_packet(pkt);
    return thread_err;
}

void *thread_test(void *arg) {
    // get a socket
    char *port = arg;
    int err = SUCCESS;
    int err_type;
    int server_sock = get_server_sock(NULL, port, NULL, &err, &err_type);
    if (server_sock == FAILURE) {
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
        return (void *)FAILURE;
    }

    long thread_err = SUCCESS;
    for (int i = 0; i < TEST_COUNT; i++) {
        if ((thread_err = test_repeat(server_sock)) != SUCCESS) {
            break;
        }
        if ((thread_err = test_ltr_count(server_sock)) != SUCCESS) {
            break;
        }
    }

    // close the socket
    close(server_sock);
    return (void *)thread_err;
}

void close_threads(pthread_t *threads, size_t size) {
    for (size_t i = 0; i < size; i++) {
        int thread_err;
        pthread_join(threads[i], (void **)&thread_err);
        if (thread_err != SUCCESS) {
            CU_FAIL("Thread failed to execute correctly.");
        }
    }
}

void test_use_counter() {
    // create threads to test the server
    pthread_t threads[PORT_RANGE];
    size_t num_threads = 0;
    for (int i = 0; i < PORT_RANGE; i++) {
        char port[6];
        snprintf(port, sizeof(port), "%d", PORT_BASE + i);
        int err = pthread_create(&threads[i], NULL, thread_test, port);
        if (err) {
            fprintf(stderr, "pthread_create: %s\n", strerror(err));
            break;
        }
        num_threads++;
    }

    // get results from threads
    close_threads(threads, num_threads);
}

int main(void) {
    CU_TestInfo suite1_tests[] = {
        {"Testing test_use_counter():", test_use_counter},
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
