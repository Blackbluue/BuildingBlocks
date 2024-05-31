#ifndef SECURE_H
#define SECURE_H

/* DATA */

#define TCP_PORT "53466" // default port for TCP

enum {
    SVR_SUCCESS = 99, // server response flag: success
    SVR_INVALID,      // server response flag: invalid request
    RQU_MSG,          // client request flag: send message
};

/* FUNCTIONS */

/**
 * @brief Allow the server to exit gracefully.
 *
 * Sets up signal handling to allow the server to exit gracefully.
 */
void allow_graceful_exit(void);

#endif /* SECURE_H */
