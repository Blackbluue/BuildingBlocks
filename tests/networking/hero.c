#include "hero.h"
#include "buildingblocks.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define SUCCESS 0
#define FAILURE -1
#define FILE_WRITE_FLAGS O_WRONLY | O_CREAT | O_APPEND
#define FILE_READ_FLAGS O_RDONLY | O_NONBLOCK
#define FILE_MODE S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH

#define HERO_FILE "hero.dat"

/* PRIVATE FUNCTIONS */

static int load_hero(const struct packet *pkt, int sock) {
    DEBUG_PRINT("looking for hero: %s\n", (char *)pkt->data);
    int fd = open(HERO_FILE, FILE_READ_FLAGS);
    if (fd < 0) {
        // log error locally
        if (errno == ENOENT) { // file does not exist
            DEBUG_PRINT("hero file does not exist\n");
            return write_pkt_data(sock, NULL, 0, SVR_NOT_FOUND);
        } else { // other error opening file
            DEBUG_PRINT("error opening hero file: %s\n", strerror(errno));
            return write_pkt_data(sock, NULL, 0, SVR_FAILURE);
        }
    }

    while (true) {
        int err;
        struct packet *hero_pkt = read_pkt(fd, &err);
        if (hero_pkt == NULL) {
            // log error locally
            close(fd);
            if (err == EAGAIN) {
                DEBUG_PRINT("ran out of heros\n"); // debug
                return write_pkt_data(sock, NULL, 0, SVR_NOT_FOUND);
            } else {                                 // other error opening file
                DEBUG_PRINT("error reading hero\n"); // debug
                return write_pkt_data(sock, NULL, 0, SVR_FAILURE);
            }
        }

        struct hero *hero = (struct hero *)hero_pkt->data;
        DEBUG_PRINT("checking hero: %s\n", hero->name);
        size_t cmp_len = pkt->hdr->data_len < strlen(hero->name)
                             ? pkt->hdr->data_len
                             : strlen(hero->name);
        if (strncmp(hero->name, pkt->data, cmp_len) != 0) {
            free_packet(hero_pkt);
            continue;
        }

        close(fd);
        DEBUG_PRINT("hero found\n\tname-> %s\n\tlevel-> %hu\n\thealth-> "
                    "%hu\n\tattack-> %hd\n\texp-> %hhu\n\tstatus-> %hhu\n",
                    hero->name, hero->level, hero->health, hero->attack,
                    hero->experience, hero->status);
        err = write_pkt_data(sock, hero, hero_pkt->hdr->data_len, SVR_SUCCESS);
        free_packet(hero_pkt);
        return err;
    }
}

/**
 * @brief Save a hero to a file.
 *
 * @param pkt - the packet containing the hero
 * @return uint32_t - SVR_SUCCESS on success, SVR_FAILURE on failure
 */
static uint32_t save_hero(const struct packet *pkt) {
    DEBUG_PRINT("saving hero to file\n");
    int fd = open(HERO_FILE, FILE_WRITE_FLAGS, FILE_MODE);
    if (fd < 0) {
        // log error locally
        return SVR_FAILURE;
    }
    // should check hero is formatted correctly
    struct hero *hero = (struct hero *)pkt->data;
    int res = write_pkt_data(fd, hero, pkt->hdr->data_len, NO_FLAGS);
    close(fd);
    // log error locally
    return res == FAILURE ? SVR_FAILURE : SVR_SUCCESS;
}

/* PUBLIC FUNCTIONS */

int init_hero(struct hero *hero, uint16_t level) {
    if (hero == NULL || level < 1 || level > 100) {
        errno = EINVAL;
        return FAILURE;
    }
    memset(hero, 0, sizeof(*hero));
    hero->level = level;
    hero->health = level * 10;
    hero->attack = level * 2;
    return SUCCESS;
}

ssize_t hero_size(const struct hero *hero) {
    return sizeof(*hero) - sizeof(hero->name) + strlen(hero->name) + 1;
}

int send_response(struct packet *pkt, struct sockaddr *addr, socklen_t addrlen,
                  int client_sock) {
    (void)addr;
    (void)addrlen;
    if (pkt == NULL) {
        return EINVAL;
    }

    switch (pkt->hdr->data_type) {
    case RQU_GET_HRO:
        return load_hero(pkt, client_sock);
    case RQU_STR_HRO:
        return write_pkt_data(client_sock, NULL, 0, save_hero(pkt));
    default:
        return write_pkt_data(client_sock, NULL, 0, SVR_INVALID);
    }
}
