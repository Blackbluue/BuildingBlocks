#define _DEFAULT_SOURCE
#include "utils.h"
#include "buildingblocks.h"
#include <ctype.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* DATA */

#define SUCCESS 0
#define ALPHABET_LEN 26

/* PRIVATE FUNCTIONS */

static size_t alpha_idx(char c) { return c - 'a'; }

static int count_letters(const struct packet *pkt, int sock) {
    struct counter_packet *req = pkt->data;
    get_highest(req->string, &req->character, &req->count);
    return write_pkt_data(sock, req, sizeof(*req), SVR_SUCCESS);
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

void get_highest(char *string, char *character, uint16_t *count) {
    char letters[ALPHABET_LEN] = {0};
    char highest = string[0];
    for (size_t i = 0; i < strlen(string); i++) {
        char cur_letter = tolower((unsigned char)string[i]);
        size_t idx = alpha_idx(cur_letter);
        letters[idx]++;
        if (letters[idx] > letters[alpha_idx(highest)]) {
            highest = cur_letter;
        }
    }

    *character = highest;
    *count = letters[alpha_idx(highest)];
}
