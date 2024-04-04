#include "serialization.h"
#include "buildingblocks.h"
#include <errno.h>
#include <stdint.h>
#include <unistd.h>

#ifdef DEBUG
#include <string.h>
#endif

/* DATA */

#define SUCCESS 0

/* PUBLIC FUNCTIONS*/

int read_exact(int fd, void *buff, size_t read_sz) {
    uint8_t *buf_ptr = (uint8_t *)buff;
    size_t total_len = 0;
    ssize_t bytes_read;
    DEBUG_PRINT("expecting %zu bytes\n", read_sz);
    do {
        if ((bytes_read = read(fd, buf_ptr, read_sz)) < 0) {
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
