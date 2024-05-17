#define _POSIX_C_SOURCE 200112L
#include "hero.h"
#include "networking_client.h"
#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SUCCESS 0
#define FAILURE -1
#define HERO_STATUS POISONED | BURNED | PARALYZED | BLINDED
#define TIMEOUT TO_DEFAULT * 5

#define HERO_NAME "Tartaglia"
io_info_t *server_io;

static void exit_handler(int sig) { (void)sig; }

void allow_graceful_exit(void) {
    struct sigaction action;
    action.sa_handler = exit_handler;
    action.sa_flags = 0;
    sigemptyset(&action.sa_mask);
    sigaction(SIGPIPE, &action, NULL);
}

int init_suite1(void) {
    int err;
    int err_type;
    server_io = get_server_info("", TCP_PORT, &err, &err_type);
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
    return SUCCESS;
}

void test_send_hero() {
    struct hero adventurer;
    CU_ASSERT_EQUAL_FATAL(init_hero(&adventurer, 90), SUCCESS);
    strncpy(adventurer.name, HERO_NAME, strlen(HERO_NAME) + 1);
    adventurer.status = HERO_STATUS;

    CU_ASSERT_EQUAL_FATAL(write_pkt_data(server_io, &adventurer,
                                         hero_size(&adventurer), RQU_STR_HRO),
                          SUCCESS);

    int err;
    struct packet *pkt = recv_pkt_data(server_io, TIMEOUT, &err);
    if (pkt == NULL) {
        CU_ASSERT_PTR_NOT_NULL_FATAL(pkt); // ensure failure
    }
    if (pkt->hdr->data_type != SVR_SUCCESS &&
        pkt->hdr->data_type != SVR_FAILURE) {
        CU_ASSERT_EQUAL(pkt->hdr->data_type, SVR_SUCCESS); // ensure failure
    }
    CU_ASSERT_EQUAL(pkt->hdr->data_len, 0);
    CU_ASSERT_EQUAL(pkt->data, NULL);
    free_packet(pkt);
}

void test_recv_hero() {
    CU_ASSERT_EQUAL_FATAL(write_pkt_data(server_io, HERO_NAME,
                                         strlen(HERO_NAME) + 1, RQU_GET_HRO),
                          SUCCESS);

    int err;
    struct packet *pkt = recv_pkt_data(server_io, TIMEOUT, &err);
    if (pkt == NULL) {
        CU_ASSERT_PTR_NOT_NULL_FATAL(pkt); // ensure failure
    }

    if (pkt->hdr->data_type != SVR_SUCCESS &&
        pkt->hdr->data_type != SVR_FAILURE &&
        pkt->hdr->data_type != SVR_NOT_FOUND) {
        CU_ASSERT_EQUAL(pkt->hdr->data_type, SVR_SUCCESS); // ensure failure
    }
    size_t expected_sz = sizeof(struct hero) - MAX_NAME_LEN + strlen(HERO_NAME);
    CU_ASSERT_EQUAL(pkt->hdr->data_len, expected_sz);
    struct hero adventurer = {0};
    memcpy(&adventurer, pkt->data, pkt->hdr->data_len);
    free_packet(pkt);

    CU_ASSERT_EQUAL(adventurer.level, 90);
    CU_ASSERT_EQUAL(adventurer.health, 900);
    CU_ASSERT_EQUAL(adventurer.attack, 180);
    CU_ASSERT_EQUAL(adventurer.experience, 0);
    CU_ASSERT_EQUAL(adventurer.status, HERO_STATUS);
}

int main(void) {
    allow_graceful_exit();
    CU_TestInfo suite1_tests[] = {
        {"Testing test_send_hero():", test_send_hero},
        {"Testing test_recv_hero():", test_recv_hero},
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
