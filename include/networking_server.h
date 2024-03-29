#ifndef NETWORKING_SERVER_H
#define NETWORKING_SERVER_H

#include "networking_utils.h"

/* DATA */

enum err_type {
    SYS,    // system error
    GAI,    // getaddrinfo error
    SOCK,   // socket error
    BIND,   // bind error
    LISTEN, // listen error
};

typedef struct server server_t;

/* FUNCTIONS */

/**
 * @brief Initialize a server.
 *
 * Creates the server object, which can be used to run multiple services.
 *
 * Possible errors:
 * - ENOMEM: Insufficient memory is available.
 *
 * @param err - the error code
 * @return server_t* - the server on success, NULL on failure
 */
server_t *init_server(int *err);

/**
 * @brief Destroy a server.
 *
 * @param server - the server to destroy
 * @return int - 0 on success, -1 on failure
 */
int destroy_server(server_t *server);

/**
 * @brief Open an Inet server socket.
 *
 * Creates an Inet socket with the given attributes and stores it in the server
 * with the given name. The socket will be bound to all interfaces. If the
 * socket type is SOCK_STREAM or SOCK_SEQPACKET, the socket will be set to
 * listen for incoming connections.
 *
 * NOTE: Until multiple services are supported, the name will be ignored and
 * calling this function multiple times will overwrite the previous service.
 *
 * Errors are separated into different types. The error type will be stored in
 * optional err_type argument, while the error itself will always be returned.
 * Possible errors:
 * - SOCK: socket error
 *      See socket(2)for more details.
 * - GAI: getaddrinfo error
 *      See getaddrinfo(3)for more details.
 * - SYS: system error
 *      EINVAL: server, name, or port is NULL
 *      ENOMEM: Insufficient memory is available.
 * - BIND: bind error
 *      See bind(2)for more details.
 * - LISTEN: listen error
 *      See listen(2)for more details.
 *
 * @param server - the server to store the socket
 * @param name - the identifier of the service
 * @param port - the port number
 * @param attr - the attributes for the server
 * @param err_type - the error code type
 * @return int - 0 on success, non-zero on failure
 */
int open_inet_socket(server_t *server, const char *name, const char *port,
                     const networking_attr_t *attr, int *err_type);

/**
 * @brief Open a Unix domain server socket.
 *
 * Creates a Unix domain server socket with the given attributes. The socket
 * will be bound to all interfaces. If the socket type is SOCK_STREAM or
 * SOCK_SEQPACKET, the socket will be set to listen for incoming connections.
 *
 * NOTE: Until multiple services are supported, the name will be ignored and
 * calling this function multiple times will overwrite the previous service.
 *
 * Possible errors:
 * - EINVAL: server, name, or path is NULL
 * - ENOMEM: Insufficient memory is available.
 * See socket(2), bind(2) and listen(2) for more error details.
 *
 * @param server - the server to store the socket
 * @param name - the identifier of the service
 * @param path - the unix domain path
 * @param attr - the attributes for the server
 * @return int - 0 on success, non-zero on failure
 */
int open_unix_socket(server_t *server, const char *name, const char *path,
                     const networking_attr_t *attr);

#endif /* NETWORKING_SERVER_H */
