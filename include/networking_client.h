#ifndef NETWORKING_CLIENT_H
#define NETWORKING_CLIENT_H

#include "networking_utils.h"

/* DATA */

/* FUNCTIONS */

int get_server_sock(const char *host, const char *port,
                    const networking_attr_t *attr, int *err, int *err_type);

#endif /* NETWORKING_CLIENT_H */
