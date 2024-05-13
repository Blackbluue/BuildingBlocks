#include "buildingblocks.h"
#include "networking_utils.h"
#include "serialization.h"
#include <errno.h>
#include <netdb.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>

/* DATA */

#define SUCCESS 0

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

/* PRIVATE FUNCTION */

/**
 * @brief Read packet header from a socket descriptor.
 *
 * @param sock - the file descriptor
 * @param hdr - pointer to store the header
 * @return int - 0 on success, non-zero on failure
 */
static int read_hdr_data(int fd, struct pkt_hdr *hdr) {
    DEBUG_PRINT("reading header...\n");
    ssize_t err = read_exact(fd, hdr, sizeof(*hdr));
    if (err != SUCCESS) {
        return err;
    }

    hdr->header_len = ntohl(hdr->header_len);
    if (hdr->header_len != sizeof(*hdr)) {
        // invalid value in reported header length
        DEBUG_PRINT("error in reported size: %s\n", strerror(err));
        return EINVAL;
    }
    hdr->data_len = ntohl(hdr->data_len);
    hdr->data_type = ntohl(hdr->data_type);

    DEBUG_PRINT("header successfully read: header_len -> %u\tdata_len -> "
                "%u\tdata_type -> %u\n",
                hdr->header_len, hdr->data_len, hdr->data_type);

    return SUCCESS;
}

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

int write_pkt_data(int fd, void *data, size_t len, uint32_t data_type) {
    DEBUG_PRINT("writing packet...\n");
    struct pkt_hdr hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.header_len = htonl(sizeof(hdr));
    hdr.data_len = htonl(len);
    hdr.data_type = htonl(data_type);

    int err = write_all(fd, &hdr, sizeof(hdr));
    if (err != SUCCESS) {
        return err;
    }
    DEBUG_PRINT("header successfully written\n");
    return write_all(fd, data, len);
}

struct packet *read_pkt(int fd, int *err) {
    struct packet *pkt = calloc(1, sizeof(*pkt));
    if (pkt == NULL) {
        set_err(err, errno);
        DEBUG_PRINT("failed to allocate packet buffer: %s\n", strerror(*err));
        return NULL;
    }
    pkt->hdr = malloc(sizeof(*pkt->hdr));
    if (pkt->hdr == NULL) {
        set_err(err, errno);
        DEBUG_PRINT("failed to allocate header buffer: %s\n", strerror(*err));
        free_packet(pkt);
        return NULL;
    }

    int loc_err = read_hdr_data(fd, pkt->hdr);
    if (loc_err != SUCCESS) {
        set_err(err, loc_err);
        free_packet(pkt);
        return NULL;
    } else if (pkt->hdr->data_len == 0) {
        DEBUG_PRINT("header successfully read: but no data\n");
        pkt->data = NULL;
        return pkt; // no data to read
    }

    pkt->data = malloc(pkt->hdr->data_len);
    if (pkt->data == NULL) {
        set_err(err, errno);
        DEBUG_PRINT("failed to allocate data buffer: %s\n", strerror(*err));
        free_packet(pkt);
        return NULL;
    }
    DEBUG_PRINT("reading data...\n");
    loc_err = read_exact(fd, pkt->data, pkt->hdr->data_len);
    if (loc_err != SUCCESS) {
        set_err(err, loc_err);
        free_packet(pkt);
        return NULL;
    }
    DEBUG_PRINT("data successfully read\n");
    return pkt;
}

struct packet *recv_pkt_data(int sock, int timeout, int *err) {
    struct pollfd pfd = {.fd = sock, .events = POLLIN};
    int loc_err = poll(&pfd, 1, timeout);
    if (loc_err <= 0) {
        // 0 on timeout, negative on poll error
        set_err(err, loc_err == 0 ? ETIMEDOUT : errno);
        DEBUG_PRINT("poll error: %s\n", strerror(*err));
        return NULL;
    } else if (pfd.revents & POLLIN) {
        DEBUG_PRINT("receiving packet...\n");
        return read_pkt(sock, err);
    } else {
        return NULL; // error in revents, usually means other end closed
    }
}
