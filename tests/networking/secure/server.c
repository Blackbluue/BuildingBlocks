#define _POSIX_C_SOURCE 200112L
#include "secure_utils_server.h"
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SUCCESS 0

int main(void) {
    int err;
    server_t *server = init_server(1, &err);
    if (server == NULL) {
        fprintf(stderr, "init_server: %s\n", strerror(err));
        return EXIT_FAILURE;
    }

    int err_type;
    char *service_name = "SSL echo server";
    err = open_inet_socket(server, service_name, TCP_PORT, &err_type);
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
    err = register_service(server, service_name, send_response, ENABLE_SSL);
    if (err) {
        fprintf(stderr, "register_server: %s\n", strerror(err));
        destroy_server(server);
        return EXIT_FAILURE;
    }

    allow_graceful_exit();
    err = run_service(server, service_name);
    if (err != SUCCESS && err != EINTR) {
        fprintf(stderr, "run_service: %s\n", strerror(err));
        err = EXIT_FAILURE;
    }
    puts("Closing server...");
    destroy_server(server);
    return err;
}
