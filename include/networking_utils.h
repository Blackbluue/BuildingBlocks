#ifndef NETWORKING_UTILS_H
#define NETWORKING_UTILS_H

#include "networking_types.h"
#include <arpa/inet.h>
#include <stdint.h>

/* DATA */

#define MAX_CONNECTIONS 1 // maximum number of pending connections
#define TO_INFINITE -1    // infinite timeout for recv_all_data

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

typedef int (*service_f)(struct packet *pkt, struct sockaddr_in *addr,
                         socklen_t addrlen, int client_sock);

/* FUNCTIONS */

/**
 * @brief Initialize the networking attributes.
 *
 * Initializes the networking attributes to their default values. Memory for the
 * attributes must be allocated before calling this function. If the attributes
 * are NULL, nothing will be done.
 *
 * Currently, this function will never fail.
 *
 * @param attr - the attributes to initialize
 * @return int - 0 on success
 */
int init_attr(networking_attr_t *attr);

/**
 * @brief Get the flags for the networking attributes.
 *
 * Gets the flags for the networking attributes. If the attributes are NULL or
 * the flags are NULL, nothing will be done.
 *
 * @param attr - the attributes to get the flags from
 * @param flags - the flags to get
 * @return int - 0 on success
 */
int network_attr_get_flags(const networking_attr_t *attr, uint32_t *flags);

/**
 * @brief Set the flags for the networking attributes.
 *
 * Sets the flags for the networking attributes. If the attributes are NULL, the
 * flags will not be set.
 *
 * @param attr - the attributes to set the flags for
 * @param flags - the flags to set
 * @return int - 0 on success
 */
int network_attr_set_flags(networking_attr_t *attr, uint32_t flags);

/**
 * @brief Get the address family for the networking attributes.
 *
 * Gets the address family for the networking attributes. If the attributes are
 * NULL or the family is NULL, nothing will be done.
 *
 * @param attr - the attributes to get the family from
 * @param family - the family to get
 * @return int - 0 on success
 */
int network_attr_get_family(const networking_attr_t *attr, int *family);

/**
 * @brief Set the address family for the networking attributes.
 *
 * Sets the address family for the networking attributes. If the attributes are
 * NULL, the family will not be set.
 *
 * @param attr - the attributes to set the family for
 * @param family - the family to set
 * @return int - 0 on success
 */
int network_attr_set_family(networking_attr_t *attr, int family);

/**
 * @brief Get the socket type for the networking attributes.
 *
 * Gets the socket type for the networking attributes. If the attributes are
 * NULL or the socket type is NULL, nothing will be done.
 *
 * @param attr - the attributes to get the socket type from
 * @param socktype - the socket type to get
 * @return int - 0 on success
 */
int network_attr_get_socktype(const networking_attr_t *attr, int *socktype);

/**
 * @brief Set the socket type for the networking attributes.
 *
 * Sets the socket type for the networking attributes. If the attributes are
 * NULL, the socket type will not be set.
 *
 * @param attr - the attributes to set the socket type for
 * @param socktype - the socket type to set
 * @return int - 0 on success
 */
int network_attr_set_socktype(networking_attr_t *attr, int socktype);

/**
 * @brief Get the maximum number of connections for the networking attributes.
 *
 * Gets the maximum number of connections for the networking attributes. If the
 * attributes are NULL or the connections is NULL, nothing will be done.
 *
 * @param attr - the attributes to get the connections from
 * @param connections - the connections to get
 * @return int - 0 on success
 */
int network_attr_get_max_connections(const networking_attr_t *attr,
                                     size_t *connections);

/**
 * @brief Set the maximum number of connections for the networking attributes.
 *
 * Sets the maximum number of connections for the networking attributes. If the
 * attributes are NULL, the connections will not be set.
 *
 * @param attr - the attributes to set the connections for
 * @param max_connections - the maximum number of connections
 * @return int - 0 on success
 */
int network_attr_set_max_connections(networking_attr_t *attr,
                                     size_t max_connections);

/**
 * @brief Free a packet.
 *
 * Frees the memory allocated for the packet.
 *
 * @param pkt - the packet to free
 */
void free_packet(struct packet *pkt);

/**
 * @brief Write packet data to a file descriptor.
 *
 * The data will be sent in a packet with a header containing the length of the
 * serialized data. recv_pkt_data or read_pkt should be used to read the data
 * in the proper format. The data_type flag is used to specify the type of the
 * data. The data_type flag is not checked here and should be checked by the
 * application.
 *
 * Possible errors are the same as write(2).
 *
 * @param fd - the file descriptor
 * @param data - the data to write
 * @param len - the length of the data
 * @param data_type - flag for the type of the data
 * @return int - 0 on success, non-zero on failure
 */
int write_pkt_data(int fd, void *data, size_t len, uint32_t data_type);

/**
 * @brief Receive packet data from a file descriptor.
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
 * @param fd - the file descriptor
 * @param err - the error code
 * @return struct packet* - pointer to the packet on success, NULL on failure
 */
struct packet *read_pkt(int fd, int *err);

/**
 * @brief Receive packet data from a socket.
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
 * @param sock - the socket
 * @param timeout - the timeout in milliseconds
 * @param err - the error code
 * @return struct packet* - pointer to the packet on success, NULL on failure
 */
struct packet *recv_pkt_data(int sock, int timeout, int *err);

#endif /* NETWORKING_UTILS_H */
