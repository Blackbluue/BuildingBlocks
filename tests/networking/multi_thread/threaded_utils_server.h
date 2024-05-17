#ifndef THREADED_UTILS_SERVER_H
#define THREADED_UTILS_SERVER_H

#include "networking_server.h"
#include "threaded_utils.h"

/* FUNCTIONS */

/**
 * @brief Allow the server to exit gracefully.
 *
 * Sets up signal handling to allow the server to exit gracefully.
 */
void allow_graceful_exit(void);

/**
 * @brief Add a counter to the server.
 *
 * Adds a counter service to the server. The counter is a service that counts
 * the number of times a character is repeated in a string. The counter is added
 * to the server with the given name and port.
 *
 * The function may fail for any of the errors specified for the routines
 * open_inet_socket(3) and register_service(3).
 *
 * @param server - The server to add the counter to.
 * @param name - The name of the counter.
 * @param port - The port to use for the counter.
 * @return int - 0 on success, -1 on failure.
 */
int add_counter(server_t *server, const char *name, const char *port);

#endif /* THREADED_UTILS_SERVER_H */
