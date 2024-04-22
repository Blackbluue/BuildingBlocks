#define _POSIX_C_SOURCE 200809L
#include "networking_server.h"
#include "buildingblocks.h"
#include "hash_table.h"
#include "threadpool.h"
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
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
    server_t *server;
};

struct server {
    hash_table_t *services;
    threadpool_t *pool;
    sigset_t oldset;
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
        DEBUG_PRINT("new_service: service %s already exists\n", name);
        return EEXIST;
    }
    *srv = calloc(1, sizeof(**srv));
    if (*srv == NULL) {
        DEBUG_PRINT("new_service: calloc failed\n");
        return ENOMEM;
    }
    (*srv)->sock = FAILURE; // to signify the socket has not been created yet
    (*srv)->name = strdup(name);
    if ((*srv)->name == NULL) {
        free(*srv);
        DEBUG_PRINT("new_service: strdup failed\n");
        return ENOMEM;
    }
    int err = hash_table_set(server->services, *srv, (*srv)->name);
    if (err != SUCCESS) {
        free_service(*srv);
        DEBUG_PRINT("new_service: hash_table_set failed\n");
        return err;
    }
    (*srv)->server = server;
    return SUCCESS;
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
            DEBUG_PRINT("socket error: %s\n", strerror(err));
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
                    DEBUG_PRINT("listen error: %s\n", strerror(err));
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
            DEBUG_PRINT("bind error: %s\n", strerror(err));
        }
    }
    return err;
}

/**
 * @brief Run a single service.
 *
 * @param srv - The service to run.
 * @param unused - Unused argument.
 * @return int - 0 on success, non-zero on failure.
 */
static int run_single(struct service_info *srv, void *unused) {
    (void)unused;
    if (srv == NULL) {
        // hash table lookup failed
        DEBUG_PRINT("run_single: service not found\n");
        return ENOENT;
    }
    DEBUG_PRINT("running service %s\n", srv->name);

    // unblock signals so the service can handle them
    sigset_t set;
    pthread_sigmask(SIG_SETMASK, &srv->server->oldset, &set);
    int err = SUCCESS;
    bool keep_running = true;
    while (keep_running) {
        struct sockaddr_storage addr;
        socklen_t addrlen = sizeof(addr);
        DEBUG_PRINT("waiting for client\n");
        int client_sock = accept(srv->sock, (struct sockaddr *)&addr, &addrlen);
        if (client_sock == FAILURE) {
            err = errno;
            DEBUG_PRINT("accept error: %s\n", strerror(errno));
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
    pthread_sigmask(SIG_SETMASK, &set, NULL);
    return err;
}

static int signal_monitor(threadpool_t *pool, void *unused) {
    (void)unused;
    DEBUG_PRINT("Signal Monitor: thread %lX\n", pthread_self());
    // block all signals for the thread
    sigset_t all_set;
    sigfillset(&all_set);
    while (true) {
        // wait for any signal sent to the process
        int sig;
        sigwait(&all_set, &sig);
        // TODO: might be able to remove this check if using dedicated thread
        if (sig == CONTROL_SIGNAL) {
            DEBUG_PRINT(
                "\ton Signal Monitor thread: received control signal\n");
            break;
        }

        DEBUG_PRINT("\ton Signal Monitor thread: cancelling waits\n");
        threadpool_cancel_wait(pool);

        // allow any signal handlers set by the application to run
        DEBUG_PRINT("\ton Signal Monitor thread: resending signal '%s'\n",
                    strsignal(sig));
        // causes threads to block
        threadpool_signal_all(pool, sig);
        // unblocks all threads, causing threadpool to go idle
        threadpool_refresh(pool);
    }
    return SUCCESS;
}

int setup_monitor(server_t *server) {
    if (server == NULL) {
        DEBUG_PRINT("setup_monitor: server is NULL\n");
        return EINVAL;
    }
    // block all signals for the thread
    sigset_t all_set;
    sigfillset(&all_set);
    pthread_sigmask(SIG_SETMASK, &all_set, &server->oldset);
    // create a thread to monitor signals
    // TODO: this may need to be a dedicated thread instead of a worker
    return threadpool_add_work(server->pool, (ROUTINE)signal_monitor,
                               server->pool, NULL);
}

/* PUBLIC FUNCTIONS */

server_t *init_server(int *err) {
    DEBUG_PRINT("creating server\n");
    server_t *server = malloc(sizeof(*server));
    if (server == NULL) {
        set_err(err, errno);
        DEBUG_PRINT("init_server: malloc failed\n");
        return NULL;
    }
    server->services =
        hash_table_init(0, (FREE_F)free_service, (CMP_F)strcmp, err);
    if (server->services == NULL) {
        free(server);
        DEBUG_PRINT("init_server: hash_table_init failed\n");
        return NULL;
    }
    server->pool = threadpool_create(NULL, err);
    if (server->pool == NULL) {
        hash_table_destroy(&server->services);
        free(server);
        DEBUG_PRINT("init_server: threadpool_create failed\n");
        return NULL;
    }
    int res = setup_monitor(server);
    if (res != SUCCESS) {
        set_err(err, res);
        destroy_server(server);
        DEBUG_PRINT("init_server: setup_monitor failed\n");
        return NULL;
    }
    DEBUG_PRINT("server initialized\n");
    return server;
}

int destroy_server(server_t *server) {
    DEBUG_PRINT("destroy_server\n");
    if (server != NULL) {
        // TODO: check if server is still running
        hash_table_destroy(&server->services);
        // signal the monitor thread to stop
        kill(getpid(), CONTROL_SIGNAL);
        threadpool_destroy(server->pool, SHUTDOWN_GRACEFUL);
        pthread_sigmask(SIG_SETMASK, &server->oldset, NULL);
        free(server);
    }
    return SUCCESS;
}

int open_inet_socket(server_t *server, const char *name, const char *port,
                     const networking_attr_t *attr, int *err_type) {
    if (server == NULL || port == NULL || name == NULL) {
        set_err(err_type, SYS);
        DEBUG_PRINT("open_inet_socket: server, name, or port is NULL\n");
        return EINVAL;
    }
    DEBUG_PRINT("opening inet socket %s\n", name);
    struct service_info *srv = NULL;
    int err = new_service(server, name, &srv);
    if (err != SUCCESS) {
        set_err(err_type, SYS);
        return err;
    }

    if (attr == NULL) {
        // use default values
        DEBUG_PRINT("open_inet_socket: using default attributes\n");
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
            DEBUG_PRINT("getaddrinfo error: %s\n", strerror(err));
        } else {
            set_err(err_type, GAI);
            DEBUG_PRINT("getaddrinfo error: %s\n", gai_strerror(err));
        }
        goto error;
    }

    err = create_socket(result, connections, &srv->sock, err_type);
    if (err == SUCCESS) {
        DEBUG_PRINT("inet socket created\n");
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
        DEBUG_PRINT("open_unix_socket: server, name, or path is NULL\n");
        return EINVAL;
    }
    DEBUG_PRINT("opening unix socket %s\n", name);
    struct service_info *srv = NULL;
    int err = new_service(server, name, &srv);
    if (err != SUCCESS) {
        return err;
    }

    if (attr == NULL) {
        // use default values
        DEBUG_PRINT("open_unix_socket: using default attributes\n");
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
    DEBUG_PRINT("unix socket created\n");
    goto cleanup;

error:
    err = errno;
    DEBUG_PRINT("open_unix_socket: %s\n", strerror(err));
    hash_table_remove(server->services, srv->name);
    free_service(srv);
cleanup: // if jumped directly here, function succeeded
    return err;
}

int register_service(server_t *server, const char *name, service_f service,
                     int flags) {
    if (server == NULL || name == NULL || service == NULL) {
        DEBUG_PRINT("register_service: server, name, or service is NULL\n");
        return EINVAL;
    }
    DEBUG_PRINT("registering service %s\n", name);
    struct service_info *srv = hash_table_lookup(server->services, name);
    if (srv == NULL) {
        DEBUG_PRINT("register_service: service %s not found\n", name);
        return ENOENT;
    }
    srv->service = service;
    srv->flags = flags;
    return SUCCESS;
}

int run_service(server_t *server, const char *name) {
    if (server == NULL || name == NULL) {
        DEBUG_PRINT("run_service: server or name is NULL\n");
        return EINVAL;
    }
    // run_single will handle the missing service error
    return run_single(hash_table_lookup(server->services, name), server->pool);
}

int run_server(server_t *server) {
    if (server == NULL) {
        DEBUG_PRINT("run_server: server is NULL\n");
        return EINVAL;
    }

    DEBUG_PRINT("starting services\n");
    int err = SUCCESS;
    bool keep_running = true;
    while (keep_running) {
        // TODO: implement run_server
        DEBUG_PRINT("waiting for services to finish\n");
        pause();
        break;
    }
    return err;
}
