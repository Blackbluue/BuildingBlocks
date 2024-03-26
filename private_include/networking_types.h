#ifndef NETWORKING_TYPES_H
#define NETWORKING_TYPES_H

#include "sizes.h"

/* DATA */

union networking_attr_t {
    char __size[NETWORKING_ATTR_SIZE];
};

#endif /* NETWORKING_TYPES_H */
