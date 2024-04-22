#ifndef NETWORKING_SERVER_H
#define NETWORKING_SERVER_H

#include "networking_utils.h"
#include <signal.h>

/* DATA */

// Used internally to control the server. Applications should not use this.
#define CONTROL_SIGNAL SIGRTMIN + 1

typedef struct server server_t;

/* FUNCTIONS */

/**
 * @brief Initialize a server.
 *
 * Creates the server object, which can be used to run multiple services.
 *
 * The server will monitor signals sent to the process. Do not alter the signal
 * mask of the process while the server is running, as this may cause the server
 * to behave unexpectedly. The mask should be altered before the server is
 * created. They will be restored when the server is destroyed. Signal handlers
 * may still be set while the server is running, and will operate as expected.
 *
 * Possible errors:
 * - ENOMEM: Insufficient memory is available.
 *
 * @param err - The error code.
 * @return server_t* - the server on success, NULL on failure.
 */
server_t *init_server(int *err);

/**
 * @brief Destroy a server.
 *
 * @param server - The server to destroy.
 * @return int - 0 on success, -1 on failure.
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
 * Errors are separated into different types. The error type will be stored in
 * optional err_type argument, while the error itself will always be returned.
 *
 * Possible errors:
 * - SOCK: socket error
 *      See socket(2)for more details.
 * - GAI: getaddrinfo error
 *      See getaddrinfo(3)for more details.
 * - SYS: system error
 *      EINVAL: server, name, or port is NULL
 *      ENOMEM: Insufficient memory is available.
 *      EEXIST: The socket with the given name already exists
 * - BIND: bind error
 *      See bind(2)for more details.
 * - LISTEN: listen error
 *      See listen(2)for more details.
 *
 * @param server - The server to store the socket.
 * @param name - The identifier of the service.
 * @param port - The port number.
 * @param attr - The attributes for the server.
 * @param err_type - The error code type.
 * @return int - 0 on success, non-zero on failure.
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
 * Possible errors:
 * - EINVAL: server, name, or path is NULL
 * - ENOMEM: Insufficient memory is available.
 * - EEXIST: The socket with the given name already exists
 * See socket(2), bind(2) and listen(2) for more error details.
 *
 * @param server - the server to store the socket.
 * @param name - the identifier of the service.
 * @param path - the unix domain path.
 * @param attr - the attributes for the server.
 * @return int - 0 on success, non-zero on failure.
 */
int open_unix_socket(server_t *server, const char *name, const char *path,
                     const networking_attr_t *attr);

/**
 * @brief Register a service with the server.
 *
 * Registers a service with the server at the given name. A socket with the
 * given name must be opened before registering a service.
 *
 * Possible errors:
 * - EINVAL: server, name, or service is NULL
 * - ENOENT: The socket with the given name does not exist
 *
 * @note The flags are currently unused and ignored.
 *
 * @param server - the server to register the service with.
 * @param name - the name of the service.
 * @param service - the service function.
 * @param flags - flags for the service.
 * @return int - 0 on success, non-zero on failure.
 */
int register_service(server_t *server, const char *name, service_f service,
                     int flags);

/**
 * @brief Run a service.
 *
 * Runs the service with the given name. The service must be registered with
 * the server before running. The function will block while accepting incoming
 * connections and running the designated service; it will return when it
 * encounters a network error or when the service returns an error.
 *
 * Possible errors:
 * - EINVAL: server or name is NULL.
 * - ENOENT: The service with the given name does not exist.
 * See accept(2) and the service function for more error details.
 *
 * @param server The server to run the service on.
 * @param name The name of the service to run.
 * @return int - 0 on success, non-zero on failure.
 */
int run_service(server_t *server, const char *name);

/**
 * @brief Run the server.
 *
 * Runs the server, accepting incoming connections and running the designated
 * services. The function will block while running the server; it will return
 * when it encounters a network error or when all services terminate.
 *
 * Possible errors:
 * - EINVAL: server is NULL.
 * See accept(2) and the service function for more error details.
 *
 * @param server - The server to run.
 * @return int - 0 on success, non-zero on failure.
 */
int run_server(server_t *server);

#endif /* NETWORKING_SERVER_H */
