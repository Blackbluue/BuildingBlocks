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
    pthread_t self;
    pthread_rwlock_t running_lock;
    size_t monitor;
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
 * @brief Initialize the server resources.
 *
 * @param max_services - The maximum number of services.
 * @param err - Where to store any errors.
 * @return server_t* - The server object on success, NULL on failure.
 */
static server_t *init_resources(size_t max_services, int *err) {
    if (max_services == 0 || max_services > MAX_THREADS - 1) {
        // minus 1 to account for the signal monitor thread
        set_err(err, EINVAL);
        DEBUG_PRINT("init_resources: max_services is invalid '%zu\n",
                    max_services);
        return NULL;
    }
    server_t *server = malloc(sizeof(*server));
    if (server == NULL) {
        set_err(err, errno);
        DEBUG_PRINT("init_server: malloc failed\n");
        return NULL;
    }
    server->services =
        hash_table_init(max_services, (FREE_F)free_service, (CMP_F)strcmp, err);
    if (server->services == NULL) {
        free(server);
        DEBUG_PRINT("init_server: hash_table_init failed\n");
        return NULL;
    }

    // create a threadpool with the maximum number of threads, created lazily
    threadpool_attr_t attr;
    threadpool_attr_init(&attr);
    // plus 1 to account for the signal monitor thread
    threadpool_attr_set_thread_count(&attr, max_services + 1);
    threadpool_attr_set_thread_creation(&attr, THREAD_CREATE_LAZY);
    server->pool = threadpool_create(&attr, err);
    threadpool_attr_destroy(&attr);
    if (server->pool == NULL) {
        hash_table_destroy(&server->services);
        free(server);
        DEBUG_PRINT("init_server: threadpool_create failed\n");
        return NULL;
    }
    pthread_rwlock_init(&server->running_lock, NULL);
    return server;
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
 * @return int - 0 on success, non-zero on failure.
 */
static int run_single(struct service_info *srv) {
    if (srv == NULL) {
        // hash table lookup failed
        DEBUG_PRINT("\ton thread %lX: service not found\n", pthread_self());
        return ENOENT;
    }
    DEBUG_PRINT("\ton thread %lX: running service %s\n", pthread_self(),
                srv->name);

    pthread_rwlock_rdlock(&srv->server->running_lock);
    int err = SUCCESS;
    bool keep_running = true;
    while (keep_running) {
        struct sockaddr_storage addr;
        socklen_t addrlen = sizeof(addr);
        DEBUG_PRINT("\ton thread %lX: waiting for client\n", pthread_self());
        int client_sock = accept(srv->sock, (struct sockaddr *)&addr, &addrlen);
        if (client_sock == FAILURE) {
            err = errno;
            DEBUG_PRINT("\ton thread %lX: accept error: %s\n", pthread_self(),
                        strerror(errno));
            break;
        }
        fcntl(client_sock, F_SETFL, O_NONBLOCK);
        DEBUG_PRINT("\ton thread %lX: client accepted\n", pthread_self());

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
            DEBUG_PRINT("\ton thread %lX: packet successfully received\n",
                        pthread_self());

            err = srv->service(pkt, (struct sockaddr *)&addr, addrlen,
                               client_sock);
            if (err != SUCCESS) {
                keep_running = false;
                handle_client = false;
            }
            DEBUG_PRINT("\ton thread %lX: packet successfully processed\n\n",
                        pthread_self());
            free_packet(pkt);
        }
        DEBUG_PRINT("\ton thread %lX: closing client\n\n\n", pthread_self());
        close(client_sock);
    }
    pthread_rwlock_unlock(&srv->server->running_lock);
    return err;
}

/**
 * @brief Run each service in the server.
 *
 * @param name - The name of the service.
 * @param srv - The service to run.
 * @param pool - The threadpool to run the service in.
 * @return int - 0 on success, non-zero on failure.
 */
static int run_each(const char *name, struct service_info **srv,
                    threadpool_t *pool) {
    if (name == NULL || srv == NULL || *srv == NULL || pool == NULL) {
        DEBUG_PRINT("run_each: service %s has invalid arguments\n", name);
        return EINVAL;
    } else if ((*srv)->service == NULL) {
        // skip services without a service function
        DEBUG_PRINT("run_each: service %s has no service function\n", name);
        return SUCCESS;
    }
    DEBUG_PRINT("adding service %s to pool\n", name);
    int err = threadpool_add_work(pool, (ROUTINE)run_single, *srv);
    if (err != SUCCESS) {
        DEBUG_PRINT("run_each: error adding work for service %s\n", name);
    }
    return err;
}

/**
 * @brief Signal monitor thread.
 *
 * This function is used to monitor signals sent to the process. When a signal
 * is caught, the function will cancel all waiting threads in the threadpool and
 * signal all threads to unblock. The function will then wait for the next
 * signal.
 *
 * @param server - The server object.
 * @return Always returns 0.
 */
static int signal_monitor(server_t *server) {
    DEBUG_PRINT("Signal Monitor: thread %lX\n", pthread_self());
    // block all signals for the thread
    sigset_t all_set;
    sigfillset(&all_set);
    sigdelset(&all_set, CONTROL_SIGNAL_2);
    while (true) {
        // wait for any signal sent to the process
        int sig;
        sigwait(&all_set, &sig);
        DEBUG_PRINT("\ton Signal Monitor thread: caught signal '%s'\n",
                    strsignal(sig));
        // TODO: might be able to remove this check if using dedicated thread
        if (sig == CONTROL_SIGNAL_1) {
            DEBUG_PRINT(
                "\ton Signal Monitor thread: received control signal\n");
            break;
        }

        DEBUG_PRINT("\ton Signal Monitor thread: cancelling waits\n");
        threadpool_cancel_wait(server->pool);

        // causes threads to block
        threadpool_signal_all(server->pool, CONTROL_SIGNAL_2);
        // in case running single service without threads
        pthread_kill(server->self, CONTROL_SIGNAL_2);
        // unblocks all threads, causing threadpool to go idle
        threadpool_refresh(server->pool);
    }
    return SUCCESS;
}

/**
 * @brief Empty signal handler.
 *
 * @param sig - The signal number.
 */
static void empty_handler(int sig) { (void)sig; }

/**
 * @brief Set the signal handler for the control signal.
 *
 * This will set the handler for the CONTROL_SIGNAL_2 signal, which is broadcast
 * to all threads in the threadpool when the server catches any signal.
 *
 * @param handler - The signal handler.
 */
static void set_control_handler(void (*handler)(int sig)) {
    struct sigaction action;
    action.sa_handler = handler;
    action.sa_flags = 0;
    sigemptyset(&action.sa_mask);
    sigaction(CONTROL_SIGNAL_2, &action, NULL);
}

/**
 * @brief Setup the signal monitor.
 *
 * This function will create a thread to monitor signals sent to the process.
 * The thread will block all signals except CONTROL_SIGNAL_2, which is used to
 * broadcast to all threads in the threadpool.
 *
 * @param server - The server object.
 * @return int - 0 on success, non-zero on failure.
 */
static int setup_monitor(server_t *server) {
    if (server == NULL) {
        DEBUG_PRINT("setup_monitor: server is NULL\n");
        return EINVAL;
    }
    DEBUG_PRINT("setting up monitor\n");
    // block all signals for the main and new threads
    sigset_t all_set;
    sigfillset(&all_set);
    sigdelset(&all_set, CONTROL_SIGNAL_2); // allow signal monitor to interrupt
    pthread_sigmask(SIG_SETMASK, &all_set, &server->oldset);
    // create a thread to monitor signals
    // TODO: this may need to be a dedicated thread instead of a worker
    int res =
        threadpool_add_work(server->pool, (ROUTINE)signal_monitor, server);
    if (res != SUCCESS) {
        DEBUG_PRINT("setup_monitor: error adding signal monitor\n");
        return res;
    }
    set_control_handler(empty_handler);
    server->self = pthread_self();
    // TODO: don't hardcode this. make the monitor a dedicated thread
    server->monitor = 0;
    return res;
}

/* PUBLIC FUNCTIONS */

server_t *init_server(size_t max_services, int *err) {
    DEBUG_PRINT("creating server\n");
    server_t *server = init_resources(max_services, err);
    if (server == NULL) {
        return NULL;
    }

    int res = setup_monitor(server);
    if (res != SUCCESS) {
        set_err(err, res);
        destroy_server(server);
        DEBUG_PRINT("setup_monitor failed\n");
        return NULL;
    }
    DEBUG_PRINT("server initialized\n");
    return server;
}

int destroy_server(server_t *server) {
    if (server != NULL) {
        DEBUG_PRINT("destroy_server\n");
        // cant acquire the write lock until all readers are done
        pthread_rwlock_wrlock(&server->running_lock);
        // all services completed once lock acquired

        // signal the monitor thread to stop
        threadpool_signal(server->pool, server->monitor, CONTROL_SIGNAL_1);
        threadpool_destroy(server->pool, SHUTDOWN_GRACEFUL);

        hash_table_destroy(&server->services);

        pthread_rwlock_unlock(&server->running_lock);
        pthread_rwlock_destroy(&server->running_lock);

        pthread_sigmask(SIG_SETMASK, &server->oldset, NULL);
        set_control_handler(SIG_DFL);
        free(server);
    }
    return SUCCESS;
}

int open_inet_socket(server_t *server, const char *name, const char *port,
                     const networking_attr_t *attr, int *err_type) {
    if (server == NULL || port == NULL || name == NULL) {
        set_err(err_type, SYS);
        DEBUG_PRINT("server, name, or port is NULL\n");
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
        DEBUG_PRINT("using default attributes\n");
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
        DEBUG_PRINT("server, name, or path is NULL\n");
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
        DEBUG_PRINT("using default attributes\n");
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
        DEBUG_PRINT("server, name, or service is NULL\n");
        return EINVAL;
    }
    DEBUG_PRINT("registering service %s\n", name);
    struct service_info *srv = hash_table_lookup(server->services, name);
    if (srv == NULL) {
        DEBUG_PRINT("service %s not found\n", name);
        return ENOENT;
    }
    srv->service = service;
    srv->flags = flags;
    return SUCCESS;
}

int run_service(server_t *server, const char *name) {
    if (server == NULL || name == NULL) {
        DEBUG_PRINT("server or name is NULL\n");
        return EINVAL;
    }
    // run_single will handle the missing service error
    return run_single(hash_table_lookup(server->services, name));
}

int run_server(server_t *server) {
    if (server == NULL) {
        DEBUG_PRINT("server is NULL\n");
        return EINVAL;
    }

    DEBUG_PRINT("starting services\n");
    int err = hash_table_iterate(server->services, (ACT_TABLE_F)run_each,
                                 server->pool);
    if (err != SUCCESS) {
        DEBUG_PRINT("error running services\n");
        return err;
    }
    DEBUG_PRINT("waiting for services to finish\n");
    err = threadpool_wait(server->pool);
    return err == EAGAIN ? EINTR : err;
}
