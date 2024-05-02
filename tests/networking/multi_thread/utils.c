#define _DEFAULT_SOURCE
#include "utils.h"
#include "buildingblocks.h"
#include <netdb.h>
#include <stdio.h>
#include <string.h>

/* DATA */

#define SUCCESS 0

/* PRIVATE FUNCTIONS */

static int count_letters(const struct packet *pkt, int sock) {
    (void)pkt;
    // struct counter_packet *req = pkt->data;
    return write_pkt_data(sock, NULL, 0, SVR_FAILURE);
}

static int repeat_letters(const struct packet *pkt, int sock) {
    struct counter_packet *req = pkt->data;
    memset(req->string, req->character, req->count);
    return write_pkt_data(sock, req, sizeof(*req), SVR_SUCCESS);
}

/* PUBLIC FUNCTIONS */

int send_response(struct packet *pkt, struct sockaddr *addr, socklen_t addrlen,
                  int client_sock) {
    char host[NI_MAXHOST];
    char serv[NI_MAXSERV];
    if (getnameinfo(addr, addrlen, host, sizeof(host), serv, sizeof(serv),
                    NO_FLAGS) == SUCCESS) {
        fprintf(stderr, "host=%s, serv=%s\n", host, serv);
    }
    switch (pkt->hdr->data_type) {
    case RQU_COUNT:
        return count_letters(pkt, client_sock);
    case RQU_REPEAT:
        return repeat_letters(pkt, client_sock);
    default:
        return write_pkt_data(client_sock, NULL, 0, SVR_INVALID);
    }
}
