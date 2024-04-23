#define _POSIX_C_SOURCE 200112L
#include "hero.h"
#include "networking_server.h"
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SUCCESS 0

static void exit_handler(int sig) { (void)sig; }

void allow_graceful_exit(void) {
    struct sigaction action;
    action.sa_handler = exit_handler;
    action.sa_flags = 0;
    sigemptyset(&action.sa_mask);
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGTERM, &action, NULL);
}

int main(void) {
    int err;
    server_t *server = init_server(1, &err);
    if (server == NULL) {
        fprintf(stderr, "init_server: %s\n", strerror(err));
        return EXIT_FAILURE;
    }

    int err_type;
    err = open_inet_socket(server, "adventure", TCP_PORT, NULL, &err_type);
    if (err != SUCCESS) {
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
        case BIND:
            fprintf(stderr, "bind: %s\n", strerror(err));
            break;
        case LISTEN:
            fprintf(stderr, "listen: %s\n", strerror(err));
            break;
        }
        destroy_server(server);
        return EXIT_FAILURE;
    }
    err = register_service(server, "adventure", send_response, NO_FLAGS);
    if (err) {
        fprintf(stderr, "register_server: %s\n", strerror(err));
        destroy_server(server);
        return EXIT_FAILURE;
    }

    allow_graceful_exit();
    err = run_service(server, "adventure");
    if (err) {
        fprintf(stderr, "run_service: %s\n", strerror(err));
        destroy_server(server);
        return EXIT_FAILURE;
    }
    puts("Closing server...");
    destroy_server(server);
}
