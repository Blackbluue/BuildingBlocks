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

/**
 * @brief Create an Inet server socket.
 *
 * Creates an Inet server with the given attributes. The socket will be bound to
 * all interfaces. If the socket type is SOCK_STREAM or SOCK_SEQPACKET, the
 * socket will be set to listen for incoming connections.
 *
 * Possible errors:
 * - SOCK: socket error
 *      See socket(2)for more details.
 * - GAI: getaddrinfo error
 *      See getaddrinfo(3)for more details.
 * - SYS: system error
 *      ENOMEM: Insufficient memory is available.
 * - BIND: bind error
 *      See bind(2)for more details.
 * - LISTEN: listen error
 *      See listen(2)for more details.
 *
 * @param attr - the attributes for the server
 * @param port - the port number
 * @return server_t* - the server on success, NULL on failure
 */
server_t *create_inet_server(const char *port, const networking_attr_t *attr,
                             int *err, int *err_type);

/**
 * @brief Create a Unix domain server.
 *
 * Creates a Unix domain server with the given attributes. The socket
 * will be bound to all interfaces. If the socket type is SOCK_STREAM or
 * SOCK_SEQPACKET, the socket will be set to listen for incoming connections.
 *
 * Possible errors:
 * - ENOMEM: Insufficient memory is available.
 * See socket(2), bind(2) and listen(2) for more details.
 *
 * @param path - the unix domain path
 * @param attr - the attributes for the server
 * @param err - the error code
 * @return server_t* - the server on success, NULL on failure
 */
server_t *create_unix_server(const char *path, const networking_attr_t *attr,
                             int *err);

#endif /* NETWORKING_SERVER_H */
