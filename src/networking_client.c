#define _POSIX_C_SOURCE 200112L
#include "networking_client.h"

/* DATA */

/* PUBLIC FUNCTIONS */

io_info_t *get_server_info(const char *host, const char *port, int *err,
                           int *err_type) {
    return new_connect_io_info(host, port, err, err_type);
}
