#include "networking_utils.h"
#include "buildingblocks.h"
#include <errno.h>
#include <netdb.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>

/* DATA */

#define SUCCESS 0

/* PRIVATE FUNCTION */

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

/* PUBLIC FUNCTIONS */

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
