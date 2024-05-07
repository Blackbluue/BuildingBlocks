#define _DEFAULT_SOURCE
#include "threaded_utils_server.h"
#include <netdb.h>
#include <stdio.h>
#include <string.h>

/* DATA */

#define SUCCESS 0
#define FAILURE -1

/* PRIVATE FUNCTIONS */

static void exit_handler(int sig) { (void)sig; }

/**
 * @brief Count the number of letters in a string.
 *
 * @param pkt - The packet containing the string to process.
 * @param sock - The socket to send the response to.
 *
 * @return int - 0 on success, -1 on failure.
 */
static int count_letters(const struct packet *pkt, int sock) {
    struct counter_packet *req = pkt->data;
    get_highest(req->string, &req->character, &req->count);
    return write_pkt_data(sock, req, sizeof(*req), SVR_SUCCESS);
}

/**
 * @brief Repeat a character in a string.
 *
 * @param pkt - The packet containing the character and string to process.
 * @param sock - The socket to send the response to.
 *
 * @return int - 0 on success, -1 on failure.
 */
static int repeat_letters(const struct packet *pkt, int sock) {
    struct counter_packet *req = pkt->data;
    memset(req->string, req->character, req->count);
    return write_pkt_data(sock, req, sizeof(*req), SVR_SUCCESS);
}

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
 * @param client - The client information.
 * @return int - 0 on success, non-zero on failure.
 */
static int send_response(struct packet *pkt, struct client_info *client) {
    char host[NI_MAXHOST];
    char serv[NI_MAXSERV];
    if (getnameinfo((struct sockaddr *)&client->addr, client->addrlen, host,
                    sizeof(host), serv, sizeof(serv), NO_FLAGS) == SUCCESS) {
        fprintf(stderr, "host=%s, serv=%s\n", host, serv);
    }
    switch (pkt->hdr->data_type) {
    case RQU_COUNT:
        return count_letters(pkt, client->client_sock);
    case RQU_REPEAT:
        return repeat_letters(pkt, client->client_sock);
    default:
        return write_pkt_data(client->client_sock, NULL, 0, SVR_INVALID);
    }
}

/* PUBLIC FUNCTIONS */

void allow_graceful_exit(void) {
    struct sigaction action;
    action.sa_handler = exit_handler;
    action.sa_flags = 0;
    sigemptyset(&action.sa_mask);
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGTERM, &action, NULL);
}

int add_counter(server_t *server, const char *name, const char *port) {
    int err;
    int err_type;
    err = open_inet_socket(server, name, port, NULL, &err_type);
    if (err != SUCCESS) {
        switch (err_type) {
        case SYS:
            fprintf(stderr, "open_inet_socket: %s\n", strerror(err));
            break;
        case GAI:
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
            break;
        case SOCK:
            fprintf(stderr, "socket: %s\n", strerror(err));
            break;
        case BIND:
            fprintf(stderr, "bind: %s\n", strerror(err));
            break;
        case LISTEN:
            fprintf(stderr, "listen: %s\n", strerror(err));
            break;
        }
        return FAILURE;
    }
    err = register_service(server, name, send_response, NO_FLAGS);
    if (err) {
        fprintf(stderr, "register_server: %s\n", strerror(err));
        return FAILURE;
    }
    return SUCCESS;
}
