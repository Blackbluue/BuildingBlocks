#ifndef NETWORKING_SSL_H
#define NETWORKING_SSL_H

/* DATA */

typedef struct ssl_info ssl_info_t;

/* FUNCTIONS */

ssl_info_t *init_ssl(int sock, int *err);

void free_ssl(ssl_info_t *ssl);

#endif /* NETWORKING_SSL_H */
