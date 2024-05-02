#define _DEFAULT_SOURCE
#include "networking_client.h"
#include "utils.h"
#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#define SUCCESS 0
#define FAILURE -1
#define TIMEOUT TO_DEFAULT * 5
#define TEST_COUNT 5
#define PORT_STR_LEN 6
#define ALPHABET_LEN 26
#define RAND_BUF_SIZE 64
#define MIN_STR_LEN 15

static void exit_handler(int sig) { (void)sig; }

static char rand_letter(struct random_data *rdata) {
    int32_t letter;
    random_r(rdata, &letter);
    return 'a' + (letter % ALPHABET_LEN);
}

static size_t rand_str_len(struct random_data *rdata) {
    int32_t num;
    random_r(rdata, &num);
    return num % (MAX_STR_LEN - MIN_STR_LEN) + MIN_STR_LEN;
}

void allow_graceful_exit(void) {
    struct sigaction action;
    action.sa_handler = exit_handler;
    action.sa_flags = 0;
    sigemptyset(&action.sa_mask);
    sigaction(SIGPIPE, &action, NULL);
}

int init_suite1(void) { return SUCCESS; }

int clean_suite1(void) { return SUCCESS; }

long test_ltr_count(int server_sock, struct random_data *rdata) {
    // send a message to the server
    uint16_t len = rand_str_len(rdata);
    char *string = calloc(len + 1, sizeof(*string));
    if (string == NULL) {
        return FAILURE;
    }
    for (size_t i = 0; i < len; i++) {
        string[i] = rand_letter(rdata);
    }

    char expected;
    uint16_t count;
    get_highest(string, &expected, &count);
    fprintf(stderr, "test_ltr_count->request string: %s\n", string);
    fprintf(stderr, "test_ltr_count->request expected: '%c' <-> %hu\n",
            expected, count);

    struct counter_packet request = {0};
    snprintf(request.string, sizeof(request.string) - 1, "%s", string);

    long thread_err =
        write_pkt_data(server_sock, &request, sizeof(request), RQU_COUNT);
    if (thread_err != SUCCESS) {
        fprintf(stderr, "test_ltr_count->write_pkt_data: %s\n",
                strerror(thread_err));
        free(string);
        return thread_err;
    }

    // get a response from the server
    int err;
    struct packet *pkt = recv_pkt_data(server_sock, TIMEOUT, &err);
    if (pkt == NULL) {
        fprintf(stderr, "test_ltr_count->recv_pkt_data: %s\n", strerror(err));
        thread_err = err;
        goto cleanup;
    }

    // test the response
    if (pkt->hdr->data_type != SVR_SUCCESS &&
        pkt->hdr->data_type != SVR_FAILURE) {
        fprintf(stderr,
                "test_ltr_count->recv_pkt_data: unexpected data type\n");
        thread_err = FAILURE;
        goto cleanup;
    }
    if (pkt->data == NULL ||
        pkt->hdr->data_len != sizeof(struct counter_packet)) {
        fprintf(stderr, "test_ltr_count->recv_pkt_data: unexpected data\n");
        thread_err = FAILURE;
        goto cleanup;
    }
    struct counter_packet *response = pkt->data;
    fprintf(stderr, "test_ltr_count->response: '%c' <-> %hu\n",
            response->character, response->count);
    if (response->count != count || response->character != expected) {
        fprintf(stderr,
                "test_ltr_count->recv_pkt_data: unexpected data values\n");
        thread_err = FAILURE;
        goto cleanup;
    }

cleanup:
    free(string);
    free_packet(pkt);
    return thread_err;
}

long test_repeat(int server_sock, struct random_data *rdata) {
    // send a message to the server
    uint16_t count = rand_str_len(rdata);
    char letter = rand_letter(rdata);
    char *expected = calloc(count + 1, sizeof(*expected));
    if (expected == NULL) {
        return FAILURE;
    }
    memset(expected, letter, count);
    fprintf(stderr, "test_repeat->request letter <-> count: '%c' <-> %hu\n",
            letter, count);
    fprintf(stderr, "test_repeat->request expected: %s\n", expected);
    struct counter_packet request = {
        .count = count,
        .character = letter,
    };

    long thread_err =
        write_pkt_data(server_sock, &request, sizeof(request), RQU_REPEAT);
    if (thread_err != SUCCESS) {
        fprintf(stderr, "test_repeat->write_pkt_data: %s\n",
                strerror(thread_err));
        free(expected);
        return thread_err;
    }

    // get a response from the server
    int err;
    struct packet *pkt = recv_pkt_data(server_sock, TIMEOUT, &err);
    if (pkt == NULL) {
        fprintf(stderr, "test_repeat->recv_pkt_data: %s\n", strerror(err));
        thread_err = err;
        goto cleanup;
    }

    // test the response
    if (pkt->hdr->data_type != SVR_SUCCESS &&
        pkt->hdr->data_type != SVR_FAILURE) {
        fprintf(stderr, "test_repeat->recv_pkt_data: unexpected data type\n");
        thread_err = FAILURE;
        goto cleanup;
    }
    if (pkt->data == NULL ||
        pkt->hdr->data_len != sizeof(struct counter_packet)) {
        fprintf(stderr, "test_repeat->recv_pkt_data: unexpected data\n");
        thread_err = FAILURE;
        goto cleanup;
    }
    struct counter_packet *response = pkt->data;
    fprintf(stderr, "test_repeat->response: %s\n", response->string);
    int cmp_res = strncmp(response->string, expected, strlen(response->string));
    if (response->count != count || cmp_res != SUCCESS) {
        fprintf(stderr, "test_repeat->recv_pkt_data: unexpected data values\n");
        thread_err = FAILURE;
        goto cleanup;
    }

cleanup:
    free(expected);
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

    // random seed set from sever socket number
    struct random_data rdata = {0};
    char statebuf[RAND_BUF_SIZE];
    initstate_r(time(NULL) + server_sock, statebuf, RAND_BUF_SIZE, &rdata);

    // run tests
    long thread_err = SUCCESS;
    for (int i = 0; i < TEST_COUNT; i++) {
        if ((thread_err = test_repeat(server_sock, &rdata)) != SUCCESS) {
            fprintf(stderr, "repeat test %d failed on port %s\n", i, port);
            break;
        }
        fprintf(stderr, "repeat test %d passed on port %s\n", i, port);
        if ((thread_err = test_ltr_count(server_sock, &rdata)) != SUCCESS) {
            fprintf(stderr, "count test %d failed on port %s\n", i, port);
            break;
        }
        fprintf(stderr, "count test %d passed on port %s\n", i, port);
    }

    // close the socket
    close(server_sock);
    return (void *)thread_err;
}

void close_threads(pthread_t *threads, size_t size) {
    for (size_t i = 0; i < size; i++) {
        long thread_err;
        pthread_join(threads[i], (void **)&thread_err);
        if (thread_err != SUCCESS) {
            CU_FAIL("Thread failed to execute correctly.");
            fprintf(stderr, "Thread %zu failed to execute correctly.\n", i);
        } else {
            fprintf(stderr, "Thread %zu executed correctly.\n", i);
        }
    }
}

void test_use_counter() {
    // create threads to test the server
    pthread_t threads[PORT_RANGE];
    char ports[PORT_RANGE][PORT_STR_LEN];
    size_t num_threads = 0;
    fputs("\n", stderr);
    for (int i = 0; i < PORT_RANGE; i++) {
        char *port = ports[i];
        snprintf(port, PORT_STR_LEN, "%d", PORT_BASE + i);
        int err = pthread_create(&threads[i], NULL, thread_test, port);
        if (err) {
            fprintf(stderr, "pthread_create: %s\n", strerror(err));
            break;
        }
        fprintf(stderr, "test thread %d created to server port %s\n", i, port);
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
