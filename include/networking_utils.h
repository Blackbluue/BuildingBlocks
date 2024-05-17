#ifndef NETWORKING_UTILS_H
#define NETWORKING_UTILS_H

#include "networking_types.h"
#include "serialization.h"
#include <arpa/inet.h>
#include <stdint.h>

/* DATA */

#define TO_INFINITE -1  // infinite timeout for recv_all_data
#define TO_DEFAULT 1000 // default timeout for recv_all_data

/**
 * @brief Attributes for creating a server.
 *
 * The attributes for creating a server, which should be treated as an opaque.
 */
typedef union networking_attr_t networking_attr_t;

/**
 * @brief Packet header.
 *
 * @param header_len - the length of the header
 * @param data_len - the length of the data
 * @param data_type - the user defined data type flag
 */
struct pkt_hdr {
    uint32_t header_len; // length of the header
    uint32_t data_len;   // length of the data
    uint32_t data_type;  // user defined data type flag
};

/**
 * @brief Packet.
 *
 * @param hdr - the packet header
 * @param data - the packet data
 */
struct packet {
    struct pkt_hdr *hdr; // packet header
    void *data;          // packet data
};

/* FUNCTIONS */

/**
 * @brief Free a packet.
 *
 * Frees the memory allocated for the packet.
 *
 * @param pkt - the packet to free
 */
void free_packet(struct packet *pkt);

/**
 * @brief Write packet data to an io_info object.
 *
 * The data will be sent in a packet with a header containing the length of the
 * serialized data. recv_pkt_data or read_pkt should be used to read the data
 * in the proper format. The data_type flag is used to specify the type of the
 * data. The data_type flag is not checked here and should be checked by the
 * application.
 *
 * Possible errors are the same as write(2).
 *
 * @param io_info - the io_info object
 * @param data - the data to write
 * @param len - the length of the data
 * @param data_type - flag for the type of the data
 * @return int - 0 on success, non-zero on failure
 */
int write_pkt_data(io_info_t *io_info, void *data, size_t len,
                   uint32_t data_type);

/**
 * @brief Receive packet data from an io_info object.
 *
 * Receives packet data from the given file descriptor and returns a pointer to
 * a packet struct containing the header and data. The other end must use
 * write_pkt_data to send the data, or the read will fail.
 *
 * Possible errors:
 *      ENODATA: The socket has reached the end of the file early.
 *      EINVAL: Invalid value in reported header length
 *      ENOMEM: Out of memory.
 * See read(2) for more details.
 *
 * @param io_info - the io_info object
 * @param err - the error code
 * @return struct packet* - pointer to the packet on success, NULL on failure
 */
struct packet *read_pkt(io_info_t *io_info, int *err);

/**
 * @brief Receive packet data from an io_info object.
 *
 * Receives packet data from the given socket and returns a pointer to a packet
 * struct containing the header and data. The other end must use write_pkt_data
 * to send the data, or the read will fail.
 *
 * The only difference between this function and read_pkt is that this function
 * will block waiting on available data, with an optional timeout.
 *
 * Possible errors:
 *      ETIMEDOUT: The operation timed out.
 *      ENODATA: The socket has reached the end of the file early.
 *      EINVAL: Invalid value in reported header length
 *      ENOMEM: Out of memory.
 * See poll(2), read(2) for more details.
 *
 * @param io_info - the io_info object
 * @param timeout - the timeout in milliseconds
 * @param err - the error code
 * @return struct packet* - pointer to the packet on success, NULL on failure
 */
struct packet *recv_pkt_data(io_info_t *io_info, int timeout, int *err);

#endif /* NETWORKING_UTILS_H */
