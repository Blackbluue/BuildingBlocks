#define _POSIX_C_SOURCE 200112L
#include "secure_utils_server.h"
#include "buildingblocks.h"
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#ifdef DEBUG
#include <pthread.h>
#endif

/* DATA */

#define SUCCESS 0
#define FAILURE -1

/* PRIVATE FUNCTIONS */

static int echo_message(const struct packet *pkt, io_info_t *client) {
    char *msg = pkt->data;
    printf("Received message: %s\n", msg);
    return write_pkt_data(client, pkt->data, pkt->hdr->data_len, SVR_SUCCESS);
}

/* PUBLIC FUNCTIONS */

int send_response(io_info_t *client) {
    int err;
    bool handle_client = true;
    while (handle_client) {
        struct packet *pkt = recv_pkt_data(client, TIMEOUT_INFINITE, &err);
        if (pkt == NULL) {
            // an error here always closes client connection
            handle_client = false;
            switch (err) {
            case EWOULDBLOCK: // no data available
            case ENODATA:     // client disconnected
            case ETIMEDOUT:   // client timed out
            case EINVAL:      // invalid packet
                // send response to client
                write_pkt_data(client, NULL, 0, SVR_INVALID);
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
        case RQU_MSG:
            err = echo_message(pkt, client);
            break;
        default:
            err = write_pkt_data(client, NULL, 0, SVR_INVALID);
            break;
        }
        free_packet(pkt);
        if (err != SUCCESS) {
            handle_client = false;
        }
    }
    return err;
}
