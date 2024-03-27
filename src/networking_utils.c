#include "networking_utils.h"
#include <netdb.h>
#include <stdlib.h>

/* DATA */

#define SUCCESS 0
#define FAILURE -1

/**
 * @brief Attributes for creating a server.
 *
 * @param flags - flags for the server
 * @param family - the address family
 * @param socktype - the socket type
 * @param max_connections - the maximum number of connections
 */
struct inner_network_attr {
    uint32_t flags;
    int family;
    int socktype;
    size_t max_connections;
};

/* PUBLIC FUNCTIONS */

int init_attr(networking_attr_t *attr) {
    if (attr != NULL) {
        struct inner_network_attr *inner_attr =
            (struct inner_network_attr *)attr;
        inner_attr->flags = 0;
        inner_attr->family = AF_INET;
        inner_attr->socktype = SOCK_STREAM;
        inner_attr->max_connections = MAX_CONNECTIONS;
    }
    return SUCCESS;
}

int network_attr_get_flags(const networking_attr_t *attr, uint32_t *flags) {
    if (attr != NULL && flags != NULL) {
        *flags = ((struct inner_network_attr *)attr)->flags;
    }
    return SUCCESS;
}

int network_attr_set_flags(networking_attr_t *attr, uint32_t flags) {
    if (attr != NULL) {
        // TODO: verify flags are valid
        ((struct inner_network_attr *)attr)->flags = flags;
    }
    return SUCCESS;
}

int network_attr_get_family(const networking_attr_t *attr, int *family) {
    if (attr != NULL && family != NULL) {
        *family = ((struct inner_network_attr *)attr)->family;
    }
    return SUCCESS;
}

int network_attr_set_family(networking_attr_t *attr, int family) {
    if (attr != NULL) {
        // TODO: verify family is valid
        ((struct inner_network_attr *)attr)->family = family;
    }
    return SUCCESS;
}

int network_attr_get_socktype(const networking_attr_t *attr, int *socktype) {
    if (attr != NULL && socktype != NULL) {
        *socktype = ((struct inner_network_attr *)attr)->socktype;
    }
    return SUCCESS;
}

int network_attr_set_socktype(networking_attr_t *attr, int socktype) {
    if (attr != NULL) {
        // TODO: verify socktype is valid
        ((struct inner_network_attr *)attr)->socktype = socktype;
    }
    return SUCCESS;
}

int network_attr_get_max_connections(const networking_attr_t *attr,
                                     size_t *connections) {
    if (attr != NULL && connections != NULL) {
        *connections = ((struct inner_network_attr *)attr)->max_connections;
    }
    return SUCCESS;
}

int network_attr_set_max_connections(networking_attr_t *attr,
                                     size_t max_connections) {
    if (attr != NULL) {
        // TODO: verify max_connections is valid
        ((struct inner_network_attr *)attr)->max_connections = max_connections;
    }
    return SUCCESS;
}

void free_packet(struct packet *pkt) {
    if (pkt != NULL) {
        free(pkt->hdr);
        free(pkt->data);
        free(pkt);
    }
}
