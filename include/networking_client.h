#ifndef NETWORKING_CLIENT_H
#define NETWORKING_CLIENT_H

#include "networking_utils.h"

/* DATA */

enum err_type {
    SYS,  // system error
    GAI,  // getaddrinfo error
    SOCK, // socket error
    CONN, // connect error
};

/* FUNCTIONS */

int get_server_sock(const char *host, const char *port,
                    const networking_attr_t *attr, int *err, int *err_type);

#endif /* NETWORKING_CLIENT_H */
