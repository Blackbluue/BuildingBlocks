#define _POSIX_C_SOURCE 200809L
#include "serialization.h"
#include "buildingblocks.h"
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef DEBUG
#include <string.h>
#endif

/* DATA */

#define SUCCESS 0
#define FAILURE -1
#define MAX_CONNECTIONS 4096

struct io_info {
    int type;
    bool close_on_free;
    int fd;
    struct sockaddr_storage addr;
    socklen_t addr_len;
};

/* PRIVATE FUNCTIONS*/

/**
 * @brief Create an inet socket object.
 *
 * @param result - the result of getaddrinfo.
 * @param sock - the socket to be created.
 * @param err_type - the type of error that occurred.
 * @return int - 0 on success, non-zero on failure.
 */
static int create_socket(struct addrinfo *result, int *sock, int *err_type) {
    int err = FAILURE;
    for (struct addrinfo *res_ptr = result; res_ptr != NULL;
         res_ptr = res_ptr->ai_next) {
        *sock = socket(res_ptr->ai_family, res_ptr->ai_socktype,
                       res_ptr->ai_protocol);
        if (*sock == FAILURE) { // error caught at the end of the loop
            err = errno;
            set_err(err_type, SOCK);
            DEBUG_PRINT("socket error: %s\n", strerror(err));
            continue;
        }

        int optval = 1;
        setsockopt(*sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
        if (bind(*sock, res_ptr->ai_addr, res_ptr->ai_addrlen) == SUCCESS) {
            if (listen(*sock, MAX_CONNECTIONS) == SUCCESS) {
                err = SUCCESS; // success, exit loop
                break;
            } else { // error caught at the end of the loop
                err = errno;
                set_err(err_type, LISTEN);
                close(*sock);
                *sock = FAILURE;
                DEBUG_PRINT("listen error: %s\n", strerror(err));
                continue;
            }
        } else { // error caught at the end of the loop
            err = errno;
            set_err(err_type, BIND);
            close(*sock);
            *sock = FAILURE;
            DEBUG_PRINT("bind error: %s\n", strerror(err));
        }
    }
    return err;
}

/* PUBLIC FUNCTIONS*/

io_info_t *new_io_info(int fd, int type, int *err) {
    io_info_t *io_info = calloc(1, sizeof(*io_info));
    if (io_info == NULL) {
        set_err(err, ENOMEM);
        return NULL;
    }
    io_info->fd = fd;
    io_info->type = type;
    io_info->close_on_free = false;
    return io_info;
}

io_info_t *new_file_io_info(const char *filename, int flags, mode_t mode,
                            int *err) {
    io_info_t *io_info = malloc(sizeof(*io_info));
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

    loc_err = create_socket(result, &io_info->fd, err_type);
    if (loc_err == SUCCESS) {
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
    new_info->type = CONNECTED_IO;
    new_info->close_on_free = true;
    return new_info;
}

int read_exact(int fd, void *buff, size_t read_sz) {
    uint8_t *buf_ptr = (uint8_t *)buff;
    size_t total_len = 0;
    ssize_t bytes_read;
    DEBUG_PRINT("expecting %zu bytes\n", read_sz);
    do {
        if ((bytes_read = read(fd, buf_ptr, read_sz)) < 0) {
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

int write_all(int fd, const void *buf, size_t len) {
    ssize_t bytes_written;
    uint8_t *buf_ptr = (uint8_t *)buf;
    DEBUG_PRINT("writing %zu bytes total\n", len);
    while (len > 0) {
        if ((bytes_written = write(fd, buf_ptr, len)) < 0) {
            DEBUG_PRINT("from call to write(2): %s\n", strerror(errno));
            return errno;
        }
        DEBUG_PRINT("amount written: %zu bytes\n", bytes_written);
        buf_ptr += bytes_written;
        len -= bytes_written;
    }
    return SUCCESS;
}
