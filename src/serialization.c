#define _DEFAULT_SOURCE
#include "serialization.h"
#include "buildingblocks.h"
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* DATA */

// disable SSL
// TODO: define this in the build system instead
#ifdef SSL_AVAILABLE
#undef SSL_AVAILABLE
#endif
#define SSL_AVAILABLE 0

#define SUCCESS 0
#define FAILURE -1

struct io_info {
    int type;
    bool close_on_free;
    int fd;
    struct sockaddr_storage addr;
    socklen_t addr_len;
    char host[NI_MAXHOST];
    char serv[NI_MAXSERV];
};

/* PRIVATE FUNCTIONS*/

/**
 * @brief Create an inet socket object.
 *
 * @param result - the result of getaddrinfo.
 * @param info - the io_info object to populate.
 * @param err_type - the type of error that occurred.
 * @return int - 0 on success, non-zero on failure.
 */
static int create_socket(struct addrinfo *result, io_info_t *info,
                         int *err_type) {
    int err = FAILURE;
    for (struct addrinfo *res_ptr = result; res_ptr != NULL;
         res_ptr = res_ptr->ai_next) {
        info->fd = socket(res_ptr->ai_family, res_ptr->ai_socktype,
                          res_ptr->ai_protocol);
        if (info->fd == FAILURE) { // error caught at the end of the loop
            err = errno;
            set_err(err_type, SOCK);
            DEBUG_PRINT("socket error: %s\n", strerror(err));
            continue;
        }

        int optval = 1;
        setsockopt(info->fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
        if (bind(info->fd, res_ptr->ai_addr, res_ptr->ai_addrlen) == SUCCESS) {
            if (listen(info->fd, MAX_CONNECTIONS) == SUCCESS) {
                err = SUCCESS; // success, exit loop
                memcpy(&info->addr, res_ptr->ai_addr, res_ptr->ai_addrlen);
                info->addr_len = res_ptr->ai_addrlen;
                break;
            } else { // error caught at the end of the loop
                err = errno;
                set_err(err_type, LISTEN);
                close(info->fd);
                info->fd = FAILURE;
                DEBUG_PRINT("listen error: %s\n", strerror(err));
                continue;
            }
        } else { // error caught at the end of the loop
            err = errno;
            set_err(err_type, BIND);
            close(info->fd);
            info->fd = FAILURE;
            DEBUG_PRINT("bind error: %s\n", strerror(err));
        }
    }
    return err;
}

/**
 * @brief Read packet header from an io_info object.
 *
 * @param io_info - the io_info object to read from
 * @param hdr - pointer to store the header
 * @return int - 0 on success, non-zero on failure
 */
static int read_hdr_data(io_info_t *io_info, struct pkt_hdr *hdr) {
    DEBUG_PRINT("reading header...\n");
    ssize_t err = read_exact(io_info, hdr, sizeof(*hdr));
    if (err != SUCCESS) {
        return err;
    }

    hdr->header_len = ntohl(hdr->header_len);
    if (hdr->header_len != sizeof(*hdr)) {
        // invalid value in reported header length
        DEBUG_PRINT("error in reported header size\n");
        return EINVAL;
    }
    hdr->data_len = ntohl(hdr->data_len);
    hdr->data_type = ntohl(hdr->data_type);

    DEBUG_PRINT("header successfully read: header_len -> %u\tdata_len -> "
                "%u\tdata_type -> %u\n",
                hdr->header_len, hdr->data_len, hdr->data_type);

    return SUCCESS;
}

/* PUBLIC FUNCTIONS*/

io_info_t *new_io_info(int fd, int type, int *err) {
    io_info_t *io_info = calloc(1, sizeof(*io_info));
    if (io_info == NULL) {
        set_err(err, ENOMEM);
        return NULL;
    }
    switch (type) {
    case ACCEPT_IO:
        // failing to get host address is not a fatal error
        if (getsockname(fd, (struct sockaddr *)&io_info->addr,
                        &io_info->addr_len) == SUCCESS) {
            getnameinfo((struct sockaddr *)&io_info->addr, io_info->addr_len,
                        io_info->host, NI_MAXHOST, io_info->serv, NI_MAXSERV,
                        NI_NUMERICHOST | NI_NUMERICSERV);
        }
        break;
    case CONNECTED_IO:
        // failing to get peer address is not a fatal error
        if (getpeername(fd, (struct sockaddr *)&io_info->addr,
                        &io_info->addr_len) == SUCCESS) {
            getnameinfo((struct sockaddr *)&io_info->addr, io_info->addr_len,
                        io_info->host, NI_MAXHOST, io_info->serv, NI_MAXSERV,
                        NI_NUMERICHOST | NI_NUMERICSERV);
        }
        break;
    case FILE_IO:
        // no extra work needed
        break;
    default:
        // unknown type
        set_err(err, EINVAL);
        free(io_info);
        return NULL;
    }
    io_info->fd = fd;
    io_info->type = type;
    io_info->close_on_free = false;
    return io_info;
}

io_info_t *new_file_io_info(const char *filename, int flags, mode_t mode,
                            int *err) {
    io_info_t *io_info = calloc(1, sizeof(*io_info));
    if (io_info == NULL) {
        set_err(err, ENOMEM);
        return NULL;
    }
    io_info->fd = open(filename, flags, mode);
    if (io_info->fd == FAILURE) {
        set_err(err, errno);
        free(io_info);
        return NULL;
    }
    io_info->type = FILE_IO;
    io_info->close_on_free = true;
    return io_info;
}

io_info_t *new_accept_io_info(const char *port, int *err, int *err_type) {
    io_info_t *io_info = malloc(sizeof(*io_info));
    if (io_info == NULL) {
        set_err(err_type, SYS);
        set_err(err, ENOMEM);
        return NULL;
    }
    struct addrinfo hints = {
        .ai_family = AF_UNSPEC,
        .ai_socktype = SOCK_STREAM,
        .ai_flags = AI_PASSIVE | AI_V4MAPPED,
        .ai_protocol = 0,
    };

    struct addrinfo *result = NULL;
    int loc_err = getaddrinfo(NULL, port, &hints, &result);
    if (loc_err != SUCCESS) {
        if (loc_err == EAI_SYSTEM) {
            loc_err = errno;
            set_err(err_type, SYS);
            DEBUG_PRINT("getaddrinfo error: %s\n", strerror(loc_err));
        } else {
            set_err(err_type, GAI);
            DEBUG_PRINT("getaddrinfo error: %s\n", gai_strerror(loc_err));
        }
        goto error;
    }

    loc_err = create_socket(result, io_info, err_type);
    if (loc_err == SUCCESS) {
        getnameinfo((struct sockaddr *)&io_info->addr, io_info->addr_len,
                    io_info->host, NI_MAXHOST, io_info->serv, NI_MAXSERV,
                    NI_NUMERICHOST | NI_NUMERICSERV);
        io_info->type = ACCEPT_IO;
        io_info->close_on_free = true;
        goto cleanup;
    }

    // only get here if no address worked
error:
    set_err(err, loc_err);
    free(io_info);
    io_info = NULL;
cleanup:
    freeaddrinfo(result);
    return io_info;
}

io_info_t *new_connect_io_info(const char *host, const char *port, int *err,
                               int *err_type) {
    io_info_t *io_info = malloc(sizeof(*io_info));
    if (io_info == NULL) {
        set_err(err_type, SYS);
        set_err(err, ENOMEM);
        return NULL;
    }
    struct addrinfo hints = {
        .ai_family = AF_UNSPEC,
        .ai_socktype = SOCK_STREAM,
        .ai_flags = AI_V4MAPPED,
        .ai_protocol = 0,
    };

    int sock = FAILURE;
    struct addrinfo *result;
    int loc_err = getaddrinfo(host, port, &hints, &result);
    if (loc_err != SUCCESS) {
        if (loc_err == EAI_SYSTEM) {
            set_err(err, errno);
            set_err(err_type, SYS);
        } else {
            set_err(err, loc_err);
            set_err(err_type, GAI);
        }
        goto error;
    }

    for (struct addrinfo *res_ptr = result; res_ptr != NULL;
         res_ptr = res_ptr->ai_next) {
        sock = socket(res_ptr->ai_family, res_ptr->ai_socktype,
                      res_ptr->ai_protocol);
        if (sock == FAILURE) {
            set_err(err, errno);
            set_err(err_type, SOCK);
            continue;
        }

        if (connect(sock, res_ptr->ai_addr, res_ptr->ai_addrlen) != FAILURE) {
            goto cleanup;
        }

        set_err(err, errno);
        set_err(err_type, CONN);
    }
    // only get here if no address worked
error:
    close(sock); // ignore EBADF error if sock is not set
    free(io_info);
    io_info = NULL;
cleanup: // if jumped directly here, function succeeded
    freeaddrinfo(result);
    return io_info;
}

void free_io_info(io_info_t *io_info) {
    if (io_info != NULL) {
        if (io_info->close_on_free) {
            close(io_info->fd);
        }
        free(io_info);
    }
}

int io_info_fd(io_info_t *io_info, int *type) {
    set_err(type, io_info->type);
    return io_info->fd;
}

const char *io_info_host(io_info_t *io_info) { return io_info->host; }

const char *io_info_serv(io_info_t *io_info) { return io_info->serv; }

int poll_io_info(struct pollio *ios, nfds_t nfds, int timeout) {
    struct pollfd *fds = malloc(nfds * sizeof(*fds));
    if (fds == NULL) {
        return errno;
    }
    for (nfds_t i = 0; i < nfds; i++) {
        fds[i].fd = ios[i].io_info->fd;
        fds[i].events = ios[i].events;
    }
    int ret = poll(fds, nfds, timeout);
    if (ret < 0) {
        ret = errno;
    }
    for (nfds_t i = 0; i < nfds; i++) {
        ios[i].revents = fds[i].revents;
    }
    free(fds);
    return ret;
}

io_info_t *io_accept(io_info_t *io_info, int *err) {
    io_info_t *new_info = calloc(1, sizeof(*new_info));
    if (new_info == NULL) {
        set_err(err, ENOMEM);
        return NULL;
    }
    new_info->fd = accept(io_info->fd, (struct sockaddr *)&new_info->addr,
                          &new_info->addr_len);
    if (new_info->fd == FAILURE) {
        set_err(err, errno);
        free(new_info);
        return NULL;
    }
    getnameinfo((struct sockaddr *)&new_info->addr, new_info->addr_len,
                new_info->host, NI_MAXHOST, new_info->serv, NI_MAXSERV,
                NI_NUMERICHOST | NI_NUMERICSERV);
    new_info->type = CONNECTED_IO;
    new_info->close_on_free = true;
    return new_info;
}

int read_exact(io_info_t *io_info, void *buff, size_t read_sz) {
    uint8_t *buf_ptr = (uint8_t *)buff;
    size_t total_len = 0;
    ssize_t bytes_read;
    DEBUG_PRINT("expecting %zu bytes\n", read_sz);
    do {
        if ((bytes_read = read(io_info->fd, buf_ptr, read_sz)) < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // retry the read if the call would block
                continue;
            }
            DEBUG_PRINT("from call to read(2): %s\n", strerror(errno));
            return errno;
        } else if (bytes_read == 0) {
            DEBUG_PRINT("from call to read(2): %s\n", strerror(ENODATA));
            return ENODATA;
        }
        DEBUG_PRINT("amount read: %zu bytes\n", bytes_read);
        total_len += bytes_read;
        buf_ptr += bytes_read;
        read_sz -= bytes_read;
    } while (total_len < read_sz);
    return SUCCESS;
}

int write_all(io_info_t *io_info, const void *buf, size_t len) {
    ssize_t bytes_written;
    uint8_t *buf_ptr = (uint8_t *)buf;
    DEBUG_PRINT("writing %zu bytes total\n", len);
    while (len > 0) {
        if ((bytes_written = write(io_info->fd, buf_ptr, len)) < 0) {
            DEBUG_PRINT("from call to write(2): %s\n", strerror(errno));
            return errno;
        }
        DEBUG_PRINT("amount written: %zu bytes\n", bytes_written);
        buf_ptr += bytes_written;
        len -= bytes_written;
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

int write_pkt_data(io_info_t *io_info, void *data, size_t len,
                   uint32_t data_type) {
    DEBUG_PRINT("writing packet...\n");
    struct pkt_hdr hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.header_len = htonl(sizeof(hdr));
    hdr.data_len = htonl(len);
    hdr.data_type = htonl(data_type);

    int err = write_all(io_info, &hdr, sizeof(hdr));
    if (err != SUCCESS) {
        return err;
    }
    DEBUG_PRINT("header successfully written\n");
    return write_all(io_info, data, len);
}

struct packet *read_pkt(io_info_t *io_info, int *err) {
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

    int loc_err = read_hdr_data(io_info, pkt->hdr);
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
    loc_err = read_exact(io_info, pkt->data, pkt->hdr->data_len);
    if (loc_err != SUCCESS) {
        set_err(err, loc_err);
        free_packet(pkt);
        return NULL;
    }
    DEBUG_PRINT("data successfully read\n");
    return pkt;
}

struct packet *recv_pkt_data(io_info_t *io_info, int timeout, int *err) {
    struct pollio pio = {.io_info = io_info, .events = POLLIN};
    int loc_err = poll_io_info(&pio, 1, timeout);
    if (loc_err <= 0) {
        // 0 on timeout, negative on poll error
        set_err(err, loc_err == 0 ? ETIMEDOUT : -loc_err);
        DEBUG_PRINT("poll error: %s\n", strerror(*err));
        return NULL;
    } else if (pio.revents & POLLIN) {
        DEBUG_PRINT("receiving packet...\n");
        return read_pkt(io_info, err);
    } else {
        return NULL; // error in revents, usually means other end closed
    }
}
