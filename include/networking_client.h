#ifndef NETWORKING_CLIENT_H
#define NETWORKING_CLIENT_H

#include "networking_utils.h"

/* DATA */

/* FUNCTIONS */

io_info_t *get_server_info(const char *host, const char *port, int *err,
                           int *err_type);

#endif /* NETWORKING_CLIENT_H */
