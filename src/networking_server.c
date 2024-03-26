#define _POSIX_C_SOURCE 200809L
#include "networking_server.h"
#include "buildingblocks.h"
#include <errno.h>
#include <netdb.h>
#include <stdlib.h>
/* DATA */

#define SUCCESS 0
#define FAILURE -1

struct server {
    int sock;
};

/* PRIVATE FUNCTIONS */

/* PUBLIC FUNCTIONS */

server_t *create_inet_server(const char *port, const networking_attr_t *attr,
                             int *err, int *err_type) {
    server_t *server = malloc(sizeof(server_t));
    if (server == NULL) {
        set_err(err, errno);
        set_err(err_type, SYS);
        return NULL;
    }
    // TODO: verify attr is configured correctly
    int socktype;
    int family;
    size_t connections;
    network_attr_get_socktype(attr, &socktype);
    network_attr_get_family(attr, &family);
    network_attr_get_max_connections(attr, &connections);

    struct addrinfo hints = {
        .ai_family = family,
        .ai_socktype = socktype,
        .ai_flags = AI_PASSIVE | AI_ADDRCONFIG | AI_V4MAPPED,
        .ai_protocol = 0,
    };

    struct addrinfo *result = NULL;
    int loc_err = getaddrinfo(NULL, port, &hints, &result);
    if (loc_err != SUCCESS) {
        if (loc_err == EAI_SYSTEM) {
            set_err(err, errno);
            set_err(err_type, SYS);
        } else {
            set_err(err, loc_err);
            set_err(err_type, GAI);
        }
        goto error;
    }

    for (struct addrinfo *res_ptr = result; res_ptr != NULL;
         res_ptr = res_ptr->ai_next) {
        server->sock = socket(res_ptr->ai_family, res_ptr->ai_socktype,
                              res_ptr->ai_protocol);
        if (server->sock == FAILURE) {
            // error caught at the end of the loop
            set_err(err, errno);
            set_err(err_type, SYS);
            continue;
        }

        int optval = 1;
        setsockopt(server->sock, SOL_SOCKET, SO_REUSEADDR, &optval,
                   sizeof(optval));
        if (bind(server->sock, res_ptr->ai_addr, res_ptr->ai_addrlen) ==
            SUCCESS) {
            if (res_ptr->ai_socktype == SOCK_STREAM ||
                res_ptr->ai_socktype == SOCK_SEQPACKET) {
                if (listen(server->sock, connections) == FAILURE) {
                    // error caught at the end of the loop
                    set_err(err, errno);
                    set_err(err_type, SYS);
                    close(server->sock);
                    continue;
                }
            }
            goto cleanup;
        }
        // error caught at the end of the loop
        set_err(err, errno);
        set_err(err_type, SYS);
        close(server->sock);
    }
    // only get here if no address worked
error:
    free(server);
    server = NULL;
cleanup: // if jumped directly here, function succeeded
    freeaddrinfo(result);
    return server;
}
