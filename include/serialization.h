#ifndef SERIALIZATION_H
#define SERIALIZATION_H

#include <stddef.h>

/* FUNCTIONS */

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

#endif /* SERIALIZATION_H */
