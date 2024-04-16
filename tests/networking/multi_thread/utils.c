#include "utils.h"
#include "buildingblocks.h"

/* DATA */

#define SUCCESS 0

/* PRIVATE FUNCTIONS */

/* PUBLIC FUNCTIONS */

int send_response(struct packet *pkt, struct sockaddr *addr, socklen_t addrlen,
                  int client_sock) {
    (void)pkt;
    (void)addr;
    (void)addrlen;
    (void)client_sock;
    return SUCCESS;
}
