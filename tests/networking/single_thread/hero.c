#include "hero.h"
#include "buildingblocks.h"
#include <errno.h>
#include <string.h>

/* DATA */

#define SUCCESS 0
#define FAILURE -1

/* PRIVATE FUNCTIONS */

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
