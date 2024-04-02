#define _POSIX_C_SOURCE 200112L
#include "networking_client.h"
#include "buildingblocks.h"
#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

/* DATA */

#define SUCCESS 0
#define FAILURE -1

/* PUBLIC FUNCTIONS */

int get_server_sock(const char *host, const char *port,
                    const networking_attr_t *attr, int *err, int *err_type) {
    // TODO: verify attr is configured correctly
    int socktype;
    int family;
    network_attr_get_socktype(attr, &socktype);
    network_attr_get_family(attr, &family);

    struct addrinfo hints = {
        .ai_family = family,
        .ai_socktype = socktype,
        .ai_flags = AI_ADDRCONFIG | AI_V4MAPPED,
        .ai_protocol = 0,
    };

    int sock = FAILURE;
    struct addrinfo *result;
    int loc_err = getaddrinfo(host, port, &hints, &result);
    if (loc_err != SUCCESS) {
        if (loc_err == EAI_SYSTEM) {
            set_err(err, errno);
            set_err(err_type, SYS);
        } else {
            set_err(err, loc_err);
            set_err(err_type, GAI);
        }
        goto error;
    }

    for (struct addrinfo *res_ptr = result; res_ptr != NULL;
         res_ptr = res_ptr->ai_next) {
        sock = socket(res_ptr->ai_family, res_ptr->ai_socktype,
                      res_ptr->ai_protocol);
        if (sock == FAILURE) {
            set_err(err, errno);
            set_err(err_type, SOCK);
            continue;
        }

        if (connect(sock, res_ptr->ai_addr, res_ptr->ai_addrlen) != FAILURE) {
            goto cleanup;
        }

        set_err(err, errno);
        set_err(err_type, CONN);
    }
    // only get here if no address worked
error:
    close(sock); // ignore EBADF error if sock is not set
    sock = FAILURE;
cleanup: // if jumped directly here, function succeeded
    freeaddrinfo(result);
    return sock;
}
