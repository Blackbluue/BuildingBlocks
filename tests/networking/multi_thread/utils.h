#ifndef UTILS_H
#define UTILS_H

#include "networking_server.h"

/* DATA */

#define MAX_STR_LEN 61
#define NO_FLAGS 0
#define PORT_BASE 53468 // default port for counters
#define PORT_RANGE 5    // number of ports to try

enum {
    SVR_SUCCESS = 99, // server response flag: success
    SVR_FAILURE,      // server response flag: internal error
    SVR_INVALID,      // server response flag: invalid request
    RQU_REPEAT,       // client request flag: repeat character
    RQU_COUNT,        // client request flag: count characters
};

struct counter_packet {
    uint16_t count; // number of times character is repeated
    union {
        char character;               // single character to be repeated
        char string[MAX_STR_LEN + 1]; // null-terminated string
    };
};

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

/**
 * @brief Count the number of times a character is repeated in a string.
 *
 * @param string - The string to process.
 * @param character - The character that is repeated the most.
 * @param count - The number of times the character is repeated.
 */
void get_highest(char *string, char *character, uint16_t *count);

#endif /* UTILS_H */
