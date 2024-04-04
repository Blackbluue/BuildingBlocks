#ifndef HERO_H
#define HERO_H

#include "networking_client.h"
#include <stdint.h>
#include <unistd.h>

/* DATA */

// status types
#define POISONED 1 << 0
#define BURNED 1 << 1
#define FROZEN 1 << 2
#define PARALYZED 1 << 3
#define CONFUSED 1 << 4
#define ASLEEP 1 << 5
#define BLINDED 1 << 6
#define SILENCED 1 << 7

#define MAX_NAME_LEN 55
#define NO_FLAGS 0
#define TCP_PORT "53467" // default port for TCP

enum {
    SVR_SUCCESS = 99, // server response flag: success
    SVR_FAILURE,      // server response flag: internal error
    SVR_INVALID,      // server response flag: invalid request
    SVR_NOT_FOUND,    // server response flag: hero not found
    RQU_GET_HRO,      // client request flag: get hero
    RQU_STR_HRO,      // client request flag: store hero
};

/**
 * @brief The hero struct.
 *
 * The hero struct represents the player's character.
 *
 * @param level - the current level of the hero
 * @param health - the current health of the hero
 * @param attack - the attack of the hero
 * @param experience - the experience of the hero to the next level, out of 100
 * @param status - a bitfield representing the status effects on the hero
 * @param name - the name of the hero
 */
struct hero {
    uint16_t level;
    uint16_t health;
    int16_t attack;
    uint8_t experience;
    uint8_t status;
    char name[MAX_NAME_LEN + 1]; // null-terminated string
};

/* FUNCTIONS */

/**
 * @brief Initialize a hero.
 *
 * Initializes the hero with the given level. The starting health and attack
 * are based on the level.
 *
 * Possible errors:
 * - EINVAL: hero is NULL, or level is outside the range [1, 100]
 *
 * @param hero - the hero to initialize
 * @param level - the level of the hero
 * @return int - 0 on success, -1 on failure
 */
int init_hero(struct hero *hero, uint16_t level);

/**
 * @brief Get the size of the hero.
 *
 * Returns the size of the hero when serialized. The size is the sum of the
 * sizes of the fields in the hero struct, including the null byte in the name.
 *
 * Possible errors:
 * - EINVAL: hero is NULL
 *
 * @param hero - the hero
 * @return ssize_t - the size of the hero on success, -1 on failure
 */
ssize_t hero_size(const struct hero *hero);

/**
 * @brief Send a response packet to the client.
 *
 * Processes a packet sent from a client, and ends a crafted response packet
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
int send_response(struct packet *pkt, struct sockaddr_in *addr,
                  socklen_t addrlen, int client_sock);

#endif /* HERO_H */
