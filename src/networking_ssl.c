#include "networking_ssl.h"
#include "buildingblocks.h"
#include <errno.h>
#include <openssl/ssl.h>
#include <stdlib.h>

/* DATA */

struct ssl_info {
    SSL_CTX *ctx;
    SSL *ssl;
};

/* PRIVATE FUNCTION */

/* PUBLIC FUNCTION */

ssl_info_t *init_ssl(int sock, int *err) {
    ssl_info_t *info = malloc(sizeof(*info));
    if (info == NULL) {
        set_err(err, ENOMEM);
        return NULL;
    }

    info->ctx = SSL_CTX_new(TLS_method());
    SSL_CTX_set_min_proto_version(info->ctx, TLS1_3_VERSION);
    info->ssl = SSL_new(info->ctx);
    SSL_set_fd(info->ssl, sock);
    return info;
}

void free_ssl(ssl_info_t *ssl) {
    if (ssl != NULL) {
        SSL_free(ssl->ssl);
        SSL_CTX_free(ssl->ctx);
        free(ssl);
    }
}
