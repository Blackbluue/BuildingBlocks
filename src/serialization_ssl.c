#include "buildingblocks.h"
#include "serialization.h"
#include <errno.h>
#include <fcntl.h>
#include <openssl/ssl.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

#ifdef DEBUG
#include <openssl/err.h>
#include <string.h>
#endif

/* DATA */

#define SUCCESS 0
#define FAILURE -1

struct io_info {
    int fd;
    BIO *bio;
};

/* PRIVATE FUNCTIONS*/

static void DEBUG_PRINT_SSL(void) {
#ifdef DEBUG
    ERR_print_errors_fp(stderr);
#endif
}

/* PUBLIC FUNCTIONS*/

io_info_t *new_file_io_info(const char *filename, int flags, mode_t mode,
                            int *err) {
    io_info_t *io_info = malloc(sizeof(*io_info));
    if (io_info == NULL) {
        set_err(err, ENOMEM);
        return NULL;
    }
    io_info->fd = open(filename, flags, mode);
    if (io_info->fd < 0) {
        set_err(err, errno);
        free(io_info);
        return NULL;
    }
    io_info->bio = BIO_new_fd(io_info->fd, BIO_CLOSE);
    if (io_info->bio == NULL) {
        set_err(err, FAILURE); // TODO: don't know what to use for error
        close(io_info->fd);
        free(io_info);
        return NULL;
    }
    return io_info;
}

io_info_t *new_accept_io_info(const char *port, int *err, int *err_type) {
    io_info_t *io_info = malloc(sizeof(*io_info));
    if (io_info == NULL) {
        set_err(err_type, SYS);
        set_err(err, ENOMEM);
        return NULL;
    }
    if ((io_info->bio = BIO_new_accept(port)) == NULL) {
        set_err(err_type, SYS);
        set_err(err, FAILURE); // TODO: don't know what to use for error
        DEBUG_PRINT("BIO_new_accept failed\n");
        DEBUG_PRINT_SSL();
        free(io_info);
        return NULL;
    }

    BIO_set_nbio_accept(io_info->bio, true);
    BIO_set_bind_mode(io_info->bio, BIO_BIND_REUSEADDR);
    if (BIO_do_accept(io_info->bio) <= SUCCESS) {
        set_err(err_type, SYS);
        set_err(err, FAILURE); // TODO: don't know what to use for error
        DEBUG_PRINT("BIO_do_accept failed\n");
        DEBUG_PRINT_SSL();
        free(io_info);
        return NULL;
    }
    BIO_set_close(io_info->bio, BIO_CLOSE);
    BIO_get_fd(io_info->bio, &io_info->fd);

    DEBUG_PRINT("inet socket created\n");
    return io_info;
}

void free_io_info(io_info_t *io_info) {
    if (io_info != NULL) {
        BIO_free(io_info->bio);
        free(io_info);
    }
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
