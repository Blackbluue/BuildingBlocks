#define _POSIX_C_SOURCE 200112L
#include "threaded_utils_server.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* DATA */

#define SUCCESS 0

/* FUNCTIONS */

int main(void) {
    int err;
    server_t *server = init_server(PORT_RANGE, &err);
    if (server == NULL) {
        fprintf(stderr, "init_server: %s\n", strerror(err));
        return EXIT_FAILURE;
    }

    for (int i = 0; i < PORT_RANGE; i++) {
        char name[9];
        char port[6];
        snprintf(name, sizeof(name), "counter%d", i + 1);
        snprintf(port, sizeof(port), "%d", PORT_BASE + i);
        if (add_counter(server, name, port) != SUCCESS) {
            destroy_server(server);
            return EXIT_FAILURE;
        }
    }

    allow_graceful_exit();
    err = run_server(server);
    if (err != SUCCESS && err != EINTR) {
        fprintf(stderr, "run_service: %s\n", strerror(err));
        destroy_server(server);
        return EXIT_FAILURE;
    }
    puts("Closing server...");
    destroy_server(server);
}
