#ifndef NETWORKING_UTILS_H
#define NETWORKING_UTILS_H

#include "networking_types.h"
#include <stdint.h>
#include <unistd.h>

/* DATA */

#define MAX_CONNECTIONS 1 // maximum number of pending connections

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

#endif /* NETWORKING_UTILS_H */
