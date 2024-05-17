#ifndef HERO_SERVER_H
#define HERO_SERVER_H

#include "hero.h"
#include "networking_server.h"

/* FUNCTIONS */

/**
 * @brief Allow the server to exit gracefully.
 *
 * Sets up signal handling to allow the server to exit gracefully.
 */
void allow_graceful_exit(void);

/**
 * @brief Handle a client session.
 *
 * Read packets sent from a client, and sends crafted response packets back. The
 * response is based on the packet received from the client. The response is
 * sent in a packet with a header containing the length of the serialized data.
 * The other end should use recv_pkt_data to receive the data.
 *
 * The function may also fail and set errno for any of the errors specified for
 * the routines open(2) and write_pkt_data(3)
 *
 * @param client - The client information.
 * @return int - 0 on success, non-zero on failure.
 */
int send_response(struct client_info *client);

#endif /* HERO_SERVER_H */
