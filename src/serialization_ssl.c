#define _DEFAULT_SOURCE
#include "buildingblocks.h"
#include "serialization.h"
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

#ifdef DEBUG
#include <openssl/err.h>
#include <string.h>
#endif

/* DATA */

// enable SSL
// TODO: define this in the build system instead
#ifdef SSL_AVAILABLE
#undef SSL_AVAILABLE
#endif
#define SSL_AVAILABLE 1

#define SUCCESS 0
#define FAILURE -1

struct io_info {
    int type;
    int fd;
    BIO *bio;
    const char *host;
    const char *serv;
};

/* PRIVATE FUNCTIONS*/

static void DEBUG_PRINT_SSL(void) {
#ifdef DEBUG
    ERR_print_errors_fp(stderr);
#endif
}

/* PUBLIC FUNCTIONS*/

io_info_t *new_io_info(int fd, int type, int *err) {
    if (fd < 0) {
        set_err(err, EINVAL);
        return NULL;
    }
    io_info_t *io_info = calloc(1, sizeof(*io_info));
    if (io_info == NULL) {
        set_err(err, ENOMEM);
        return NULL;
    }
    io_info->fd = fd;
    switch (io_info->type = type) {
    case ACCEPT_IO:
        io_info->bio = BIO_new(BIO_s_accept());
        if (io_info->bio == NULL) {
            set_err(err, FAILURE); // TODO: don't know what to use for error
            DEBUG_PRINT("BIO_new failed\n");
            DEBUG_PRINT_SSL();
            goto error;
        }
        BIO_set_fd(io_info->bio, io_info->fd, BIO_NOCLOSE);
        io_info->host = BIO_get_accept_name(io_info->bio);
        io_info->serv = BIO_get_accept_port(io_info->bio);
        break;
    case CONNECTED_IO:
        io_info->bio = BIO_new(BIO_s_connect());
        if (io_info->bio == NULL) {
            set_err(err, FAILURE); // TODO: don't know what to use for error
            DEBUG_PRINT("BIO_new failed\n");
            DEBUG_PRINT_SSL();
            goto error;
        }
        if (BIO_set_fd(io_info->bio, io_info->fd, BIO_NOCLOSE) < 1) {
            set_err(err, FAILURE); // TODO: don't know what to use for error
            DEBUG_PRINT("BIO_set_fd failed\n");
            DEBUG_PRINT_SSL();
            goto error;
        }
        io_info->host = BIO_get_conn_hostname(io_info->bio);
        io_info->serv = BIO_get_conn_port(io_info->bio);
        break;
    case FILE_IO:
        io_info->bio = BIO_new(BIO_s_fd());
        if (io_info->bio == NULL) {
            set_err(err, FAILURE); // TODO: don't know what to use for error
            DEBUG_PRINT("BIO_new failed\n");
            DEBUG_PRINT_SSL();
            goto error;
        }
        BIO_set_fd(io_info->bio, io_info->fd, BIO_NOCLOSE);
        // files have no host or service
        break;
    default:
        set_err(err, EINVAL);
        goto error;
    }
    goto cleanup;

error:
    BIO_free(io_info->bio);
    free(io_info);
    io_info = NULL;
cleanup:
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
    io_info->type = FILE_IO;
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
    io_info->host = BIO_get_accept_name(io_info->bio);
    io_info->serv = BIO_get_accept_port(io_info->bio);
    (void)BIO_set_close(io_info->bio, BIO_CLOSE);
    BIO_get_fd(io_info->bio, &io_info->fd);
    io_info->type = ACCEPT_IO;

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

    if ((io_info->bio = BIO_new(BIO_s_connect())) == NULL) {
        set_err(err_type, SYS);
        set_err(err, FAILURE); // TODO: don't know what to use for error
        DEBUG_PRINT("BIO_new_connect failed\n");
        DEBUG_PRINT_SSL();
        free(io_info);
        return NULL;
    }
    BIO_set_conn_hostname(io_info->bio, host);
    BIO_set_conn_port(io_info->bio, port);
    BIO_set_nbio(io_info->bio, true);

    while (BIO_do_connect(io_info->bio) <= SUCCESS) {
        if (BIO_should_retry(io_info->bio)) {
            DEBUG_PRINT(
                "BIO_do_connect would block to server <-> host: %s port: %s\n",
                host, port);
            continue;
        } else {
            set_err(err_type, SYS);
            set_err(err, FAILURE); // TODO: don't know what to use for error
            DEBUG_PRINT("BIO_do_connect failed\n");
            DEBUG_PRINT_SSL();
        }
        free_io_info(io_info);
        return NULL;
    }

    io_info->host = BIO_get_conn_hostname(io_info->bio);
    io_info->serv = BIO_get_conn_port(io_info->bio);
    (void)BIO_set_close(io_info->bio, BIO_CLOSE);
    BIO_get_fd(io_info->bio, &io_info->fd);
    io_info->type = CONNECTED_IO;

    return io_info;
}

void free_io_info(io_info_t *io_info) {
    if (io_info != NULL) {
        BIO_free(io_info->bio);
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
        return -errno;
    }
    for (nfds_t i = 0; i < nfds; i++) {
        fds[i].fd = ios[i].io_info->fd;
        fds[i].events = ios[i].events;
    }
    int ret = poll(fds, nfds, timeout);
    if (ret <= 0) {
        if (ret < 0) {
            ret = -errno;
        }
        goto cleanup;
    }
    for (nfds_t i = 0; i < nfds; i++) {
        ios[i].revents = fds[i].revents;
    }
cleanup:
    free(fds);
    return ret;
}

io_info_t *io_accept(io_info_t *io_info, int *err) {
    io_info_t *new_info = malloc(sizeof(*new_info));
    if (new_info == NULL) {
        set_err(err, ENOMEM);
        return NULL;
    }

    if (BIO_do_accept(io_info->bio) <= SUCCESS) {
        set_err(err, FAILURE); // TODO: don't know what to use for error
        DEBUG_PRINT("Error accepting connection\n");
        DEBUG_PRINT_SSL();
        free(new_info);
        return NULL;
    }

    new_info->bio = BIO_pop(io_info->bio);
    BIO_set_nbio_accept(new_info->bio, true);
    (void)BIO_set_close(new_info->bio, BIO_CLOSE);
    BIO_get_fd(new_info->bio, &new_info->fd);
    new_info->host = BIO_get_conn_hostname(new_info->bio);
    new_info->serv = BIO_get_conn_port(new_info->bio);
    new_info->type = CONNECTED_IO;

    return new_info;
}

int read_exact(io_info_t *io_info, void *buff, size_t read_sz) {
    uint8_t *buf_ptr = (uint8_t *)buff;
    size_t total_len = 0;
    size_t bytes_read = 0;
    DEBUG_PRINT("expecting %zu bytes\n", read_sz);
    do {
        int ret = BIO_read_ex(io_info->bio, buf_ptr, read_sz, &bytes_read);
        if (!ret) {
            if (BIO_should_retry(io_info->bio)) {
                // retry the read if the call would block
                continue;
            }
            // DEBUG_PRINT("from call to read(2): %s\n", strerror(errno));
            DEBUG_PRINT("BIO_read_ex failed\n");
            DEBUG_PRINT_SSL();
            return FAILURE; // TODO: don't know what to use for error
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
    size_t bytes_written;
    uint8_t *buf_ptr = (uint8_t *)buf;
    DEBUG_PRINT("writing %zu bytes total\n", len);
    while (len > 0) {
        int ret = BIO_write_ex(io_info->bio, buf_ptr, len, &bytes_written);
        if (!ret) {
            DEBUG_PRINT("BIO_write_ex failed\n");
            DEBUG_PRINT_SSL();
            return FAILURE; // TODO: don't know what to use for error
        }
        DEBUG_PRINT("amount written: %zu bytes\n", bytes_written);
        buf_ptr += bytes_written;
        len -= bytes_written;
    }
    return SUCCESS;
}
