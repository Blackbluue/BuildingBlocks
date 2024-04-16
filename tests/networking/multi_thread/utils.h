#ifndef UTILS_H
#define UTILS_H

#include "networking_client.h"

/* DATA */

#define NO_FLAGS 0
#define PORT_BASE 53468 // default port for counters
#define PORT_RANGE 5    // number of ports to try

/* FUNCTIONS */

/**
 * @brief Send a response packet to the client.
 *
 * Processes a packet sent from a client, and sends a crafted response packet
 * back. The response is based on the packet received from the client. The
 * response is sent in a packet with a header containing the length of the
 * serialized data. The other end should use recv_pkt_data to receive the data.
 *
 * Possible errors:
 * - EINVAL: pkt is NULL
 * The function may also fail and set errno for any of the errors specified for
 * the routines open(2) and write_pkt_data(3)
 *
 * @param pkt - The packet to process.
 * @param addr - Unused.
 * @param addrlen - Unused.
 * @param client_sock - The socket descriptor.
 * @return int - 0 on success, non-zero on failure.
 */
int send_response(struct packet *pkt, struct sockaddr *addr, socklen_t addrlen,
                  int client_sock);

#endif /* UTILS_H */
