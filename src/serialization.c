#include "serialization.h"
#include <errno.h>
#include <stdint.h>
#include <unistd.h>

/* DATA */

#define SUCCESS 0

/* PUBLIC FUNCTIONS*/

int read_exact(int fd, void *buff, size_t read_sz) {
    uint8_t *buf_ptr = (uint8_t *)buff;
    size_t total_len = 0;
    ssize_t bytes_read;
    do {
        if ((bytes_read = read(fd, buf_ptr, read_sz)) < 0) {
            return errno;
        } else if (bytes_read == 0) {
            return ENODATA;
        }
        total_len += bytes_read;
        buf_ptr += bytes_read;
        read_sz -= bytes_read;
    } while (total_len < read_sz);
    return SUCCESS;
}
