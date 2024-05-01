#define _DEFAULT_SOURCE
#include "utils.h"
#include "buildingblocks.h"
#include <netdb.h>
#include <stdio.h>

/* DATA */

#define SUCCESS 0

/* PRIVATE FUNCTIONS */

/* PUBLIC FUNCTIONS */

int send_response(struct packet *pkt, struct sockaddr *addr, socklen_t addrlen,
                  int client_sock) {
    (void)pkt;
    (void)client_sock;
    char host[NI_MAXHOST];
    char serv[NI_MAXSERV];
    if (getnameinfo(addr, addrlen, host, sizeof(host), serv, sizeof(serv),
                    NO_FLAGS) == SUCCESS) {
        printf("host=%s, serv=%s\n", host, serv);
    }
    return SUCCESS;
}
