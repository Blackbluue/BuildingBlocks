#define _DEFAULT_SOURCE
#include "threaded_utils_server.h"
#include "buildingblocks.h"
#include <errno.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#ifdef DEBUG
#include <pthread.h>
#endif

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
static int send_response(io_info_t *client) {
    fprintf(stderr, "host=%s, serv=%s\n", io_info_host(client),
            io_info_serv(client));

    int err;
    bool handle_client = true;
    int sock = io_info_fd(client, NULL);
    while (handle_client) {
        struct packet *pkt = recv_pkt_data(sock, TO_INFINITE, &err);
        if (pkt == NULL) {
            // an error here always closes client connection
            handle_client = false;
            switch (err) {
            case EWOULDBLOCK: // no data available
            case ENODATA:     // client disconnected
            case ETIMEDOUT:   // client timed out
            case EINVAL:      // invalid packet
                // send response to client
                write_pkt_data(sock, NULL, 0, SVR_INVALID);
                continue;
            case EINTR:        // signal interrupt
                err = SUCCESS; // no error
                // fall through
            default: // other errors
                continue;
            }
        }
#ifdef DEBUG
        DEBUG_PRINT("\ton thread %lX: packet successfully received\n",
                    pthread_self());
#endif

        switch (pkt->hdr->data_type) {
        case RQU_COUNT:
            err = count_letters(pkt, sock);
            break;
        case RQU_REPEAT:
            err = repeat_letters(pkt, sock);
            break;
        default:
            err = write_pkt_data(sock, NULL, 0, SVR_INVALID);
            break;
        }
        free_packet(pkt);
        if (err != SUCCESS) {
            handle_client = false;
        }
    }
    return err;
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
    err = register_service(server, name, send_response, THREADED_SESSIONS);
    if (err) {
        fprintf(stderr, "register_server: %s\n", strerror(err));
        return FAILURE;
    }
    return SUCCESS;
}
