#ifndef SERIALIZATION_H
#define SERIALIZATION_H

#include <poll.h>
#include <stddef.h>
#include <sys/types.h>

/* DATA */

// TODO: err_type syntax is ugly, need to find an alternative
enum err_type {
    SYS,    // system error
    GAI,    // getaddrinfo error
    SOCK,   // socket error
    BIND,   // bind error
    LISTEN, // listen error
    CONN,   // connect error
};

typedef enum {
    FILE_IO,     // file i/o, used for reading and writing files
    ACCEPT_IO,   // accept i/o, used for accepting network connections
    CONNECTED_IO // connect i/o, an existing network connection
} io_type_t;

typedef struct io_info io_info_t;

struct pollio {
    io_info_t *io_info; // i/o info
    short events;       // requested events
    short revents;      // returned events
};

/* FUNCTIONS */

/**
 * @brief Create a new io_info object.
 *
 * The type of the object will determine the type of i/o that can be performed.
 * The file descriptor will be stored in the io_info object. Note that in this
 * case, the file descriptor will by default not be closed when the io_info
 * object is freed, unlike when the object is created from one of the other
 * dedicated functions.
 *
 * @param fd - The file descriptor.
 * @param type - The type of i/o.
 * @param err - Where to store the error code.
 * @return io_info_t* - The io_info object.
 */
io_info_t *new_io_info(int fd, io_type_t type, int *err);

/**
 * @brief Create a new io_info object for file operations.
 *
 * The underlying file descriptor will be closed when the io_info object is
 * freed.
 *
 * @param filename - The name of the file.
 * @param flags - The flags for opening the file.
 * @param mode - The mode for opening the file.
 * @param err - Where to store the error code.
 * @return io_info_t* - The io_info object.
 */
io_info_t *new_file_io_info(const char *filename, int flags, mode_t mode,
                            int *err);

/**
 * @brief Create a new io_info object for accepting network connections.
 *
 * Any underlying socket will be closed when the io_info object is freed.
 *
 * @param port - The port to listen on.
 * @param err - Where to store the error code.
 * @param err_type - Where to store the error type.
 * @return io_info_t* - The io_info object.
 */
io_info_t *new_accept_io_info(const char *port, int *err, int *err_type);

/**
 * @brief Free an io_info object.
 *
 * @param io_info - The io_info object to free.
 */
void free_io_info(io_info_t *io_info);

/**
 * @brief Wrapper for poll(2) that uses io_info objects.
 *
 * @param ios - The array of io_info objects.
 * @param nfds - The number of io_info objects.
 * @param timeout - The timeout in milliseconds.
 * @return int - The return value of poll(2).
 */
int poll_io_info(struct pollio *ios, nfds_t nfds, int timeout);

/**
 * @brief Accept a new connection on a socket io_info.
 *
 * @param io_info - The io_info object to accept on.
 * @param err - Where to store the error code.
 * @return io_info_t* - The new io_info object.
 */
io_info_t *io_accept(io_info_t *io_info, int *err);

/**
 * @brief Read a fixed amount of data from a file descriptor.
 *
 * If an error occurs, a non-zero error code will be returned
 * Possible errors:
 *   ENODATA: - the file descriptor has reached the end of the file early
 * See read(2) for additional possible errors.
 *
 * @param fd - the file descriptor
 * @param buff - pointer to store the data
 * @param read_sz - the size of the data to read
 * @return int - 0 on success, non-zero on failure
 */
int read_exact(int fd, void *buff, size_t read_sz);

/**
 * @brief Write all data to a file descriptor.
 *
 * The function handles partial writes and will continue to write until all the
 * data is written.
 *
 * If an error occurs, a non-zero error code will be returned
 * See write(2) for possible errors.
 *
 * @param fd - the file descriptor
 * @param buf - the data to send
 * @param len - the length of the data
 * @return int - 0 on success, non-zero on failure
 */
int write_all(int fd, const void *buf, size_t len);

#endif /* SERIALIZATION_H */
