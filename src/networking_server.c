#define _POSIX_C_SOURCE 200809L
#include "networking_server.h"
#include "buildingblocks.h"
#include "hash_table.h"
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/un.h>
#include <unistd.h>

/* DATA */

#define SUCCESS 0
#define FAILURE -1

struct service_info {
    char *name;
    int flags;
    service_f service;
    int sock;
};

struct server {
    hash_table_t *services;
};

/* PRIVATE FUNCTIONS */

/**
 * @brief Free a service object.
 *
 * @param srv - The service object to be freed.
 */
static void free_service(struct service_info *srv) {
    free(srv->name);
    if (srv->sock != FAILURE) {
        close(srv->sock);
    }
    free(srv);
}

/**
 * @brief Create a new service object and add it to the server.
 *
 * @param server - The server to add the service to.
 * @param name - The name of the service.
 * @param srv - The service object to be created.
 * @return int - 0 on success, non-zero on failure.
 */
static int new_service(server_t *server, const char *name,
                       struct service_info **srv) {
    *srv = hash_table_lookup(server->services, name);
    if (*srv != NULL) {
        return EEXIST;
    }
    *srv = calloc(1, sizeof(**srv));
    if (*srv == NULL) {
        return ENOMEM;
    }
    (*srv)->sock = FAILURE; // to signify the socket has not been created yet
    (*srv)->name = strdup(name);
    if ((*srv)->name == NULL) {
        free(*srv);
        return ENOMEM;
    }
    int err = hash_table_set(server->services, *srv, (*srv)->name);
    if (err != SUCCESS) {
        free_service(*srv);
    }
    return err;
}

/**
 * @brief Create an inet socket object.
 *
 * @param result - the result of getaddrinfo.
 * @param connections - the maximum number of connections.
 * @param sock - the socket to be created.
 * @param err_type - the type of error that occurred.
 * @return int - 0 on success, non-zero on failure.
 */
static int create_socket(struct addrinfo *result, int connections, int *sock,
                         int *err_type) {
    int err = FAILURE;
    for (struct addrinfo *res_ptr = result; res_ptr != NULL;
         res_ptr = res_ptr->ai_next) {
        *sock = socket(res_ptr->ai_family, res_ptr->ai_socktype,
                       res_ptr->ai_protocol);
        if (*sock == FAILURE) { // error caught at the end of the loop
            err = errno;
            set_err(err_type, SOCK);
            continue;
        }

        int optval = 1;
        setsockopt(*sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
        if (bind(*sock, res_ptr->ai_addr, res_ptr->ai_addrlen) == SUCCESS) {
            if (res_ptr->ai_socktype == SOCK_STREAM ||
                res_ptr->ai_socktype == SOCK_SEQPACKET) {
                if (listen(*sock, connections) == SUCCESS) {
                    err = SUCCESS; // success, exit loop
                    break;
                } else { // error caught at the end of the loop
                    err = errno;
                    set_err(err_type, LISTEN);
                    close(*sock);
                    *sock = FAILURE;
                    continue;
                }
            } else {           // don't attempt to listen
                err = SUCCESS; // success, exit loop
                break;
            }
        } else { // error caught at the end of the loop
            err = errno;
            set_err(err_type, BIND);
            close(*sock);
            *sock = FAILURE;
        }
    }
    return err;
}

/* PUBLIC FUNCTIONS */

server_t *init_server(int *err) {
    server_t *server = malloc(sizeof(*server));
    if (server == NULL) {
        set_err(err, errno);
        return NULL;
    }
    server->services =
        hash_table_init(0, (FREE_F)free_service, (CMP_F)strcmp, err);
    if (server->services == NULL) {
        free(server);
        return NULL;
    }
    return server;
}

int destroy_server(server_t *server) {
    if (server != NULL) {
        // TODO: check if server is still running
        hash_table_destroy(&server->services);
        free(server);
    }
    return SUCCESS;
}

int open_inet_socket(server_t *server, const char *name, const char *port,
                     const networking_attr_t *attr, int *err_type) {
    if (server == NULL || port == NULL || name == NULL) {
        set_err(err_type, SYS);
        return EINVAL;
    }
    struct service_info *srv = NULL;
    int err = new_service(server, name, &srv);
    if (err != SUCCESS) {
        set_err(err_type, SYS);
        return err;
    }

    if (attr == NULL) {
        // use default values
        networking_attr_t def_attr;
        attr = &def_attr;
        init_attr((networking_attr_t *)attr);
    }
    int socktype;
    int family;
    size_t connections;
    network_attr_get_socktype(attr, &socktype);
    network_attr_get_family(attr, &family);
    network_attr_get_max_connections(attr, &connections);

    struct addrinfo hints = {
        .ai_family = family,
        .ai_socktype = socktype,
        .ai_flags = AI_PASSIVE | AI_V4MAPPED,
        .ai_protocol = 0,
    };

    struct addrinfo *result = NULL;
    err = getaddrinfo(NULL, port, &hints, &result);
    if (err != SUCCESS) {
        if (err == EAI_SYSTEM) {
            err = errno;
            set_err(err_type, SYS);
        } else {
            set_err(err_type, GAI);
        }
        goto error;
    }

    err = create_socket(result, connections, &srv->sock, err_type);
    if (err == SUCCESS) {
        goto cleanup;
    }
    // only get here if no address worked
error:
    hash_table_remove(server->services, srv->name);
    free_service(srv);
cleanup:
    freeaddrinfo(result);
    return err;
}

int open_unix_socket(server_t *server, const char *name, const char *path,
                     const networking_attr_t *attr) {
    if (server == NULL || name == NULL || path == NULL) {
        return EINVAL;
    }
    struct service_info *srv = NULL;
    int err = new_service(server, name, &srv);
    if (err != SUCCESS) {
        return err;
    }

    if (attr == NULL) {
        // use default values
        networking_attr_t def_attr;
        attr = &def_attr;
        init_attr((networking_attr_t *)attr);
    }
    int socktype;
    size_t connections;
    network_attr_get_socktype(attr, &socktype);
    network_attr_get_max_connections(attr, &connections);

    err = SUCCESS;
    srv->sock = socket(AF_UNIX, socktype, 0);
    if (srv->sock == FAILURE) {
        goto error;
    }

    struct sockaddr_un addr = {
        .sun_family = AF_UNIX,
    };
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);

    if (bind(srv->sock, (struct sockaddr *)&addr, sizeof(addr)) == FAILURE) {
        goto error;
    }

    if (socktype == SOCK_STREAM || socktype == SOCK_SEQPACKET) {
        if (listen(srv->sock, connections) == FAILURE) {
            goto error;
        }
    }
    goto cleanup;

error:
    err = errno;
    hash_table_remove(server->services, srv->name);
    free_service(srv);
cleanup: // if jumped directly here, function succeeded
    return err;
}

int register_service(server_t *server, const char *name, service_f service,
                     int flags) {
    if (server == NULL || name == NULL || service == NULL) {
        return EINVAL;
    }
    struct service_info *srv = hash_table_lookup(server->services, name);
    if (srv == NULL) {
        return ENOENT;
    }
    srv->service = service;
    srv->flags = flags;
    return SUCCESS;
}

int run_service(server_t *server, const char *name) {
    if (server == NULL || name == NULL) {
        return EINVAL;
    }
    struct service_info *srv = hash_table_lookup(server->services, name);
    if (srv == NULL) {
        return ENOENT;
    }

    int err = SUCCESS;
    bool keep_running = true;
    while (keep_running) {
        struct sockaddr_storage addr;
        socklen_t addrlen = sizeof(addr);
        int client_sock = accept(srv->sock, (struct sockaddr *)&addr, &addrlen);
        if (client_sock == FAILURE) {
            if (errno != EINTR) {
                err = errno;
            }
            break;
        }
        fcntl(client_sock, F_SETFL, O_NONBLOCK);
        DEBUG_PRINT("client accepted\n");

        bool handle_client = true;
        while (handle_client) {
            struct packet *pkt = recv_pkt_data(client_sock, TO_INFINITE, &err);
            if (pkt == NULL) {
                handle_client = false; // drop the client
                switch (err) {
                case EWOULDBLOCK:  // no data available
                case ENODATA:      // client disconnected
                case ETIMEDOUT:    // client timed out
                case EINVAL:       // invalid packet
                    err = 0;       // clear error
                    continue;      // don't close the server
                case EINTR:        // signal interrupt
                    err = SUCCESS; // no error
                    // fall through
                default:                  // other errors
                    keep_running = false; // close the server
                    continue;
                }
            }
            DEBUG_PRINT("packet successfully received\n");

            err = srv->service(pkt, (struct sockaddr *)&addr, addrlen,
                               client_sock);
            if (err != SUCCESS) {
                keep_running = false;
                handle_client = false;
            }
            DEBUG_PRINT("packet successfully processed\n\n");
            free_packet(pkt);
        }
        DEBUG_PRINT("closing client\n\n\n");
        close(client_sock);
    }
    return err;
}

int run_server(server_t *server) {
    if (server == NULL) {
        return EINVAL;
    }

    int err = SUCCESS;
    bool keep_running = true;
    while (keep_running) {
        // TODO: implement run_server
        pause();
        break;
    }
    return err;
}
