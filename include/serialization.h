#ifndef SERIALIZATION_H
#define SERIALIZATION_H

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

typedef struct io_info io_info_t;

/* FUNCTIONS */

/**
 * @brief Create a new io_info object for file operations.
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
 * @param port - The port to listen on.
 * @param err - Where to store the error code.
 * @param err_type - Where to store the error type.
 * @return io_info_t* - The io_info object.
 */
io_info_t *new_accept_io_info(const char *port, int *err, int *err_type);

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
