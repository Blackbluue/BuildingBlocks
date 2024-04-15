#ifndef NETWORKING_TYPES_H
#define NETWORKING_TYPES_H

/* DATA */

#define NETWORKING_ATTR_SIZE 32

union networking_attr_t {
    char __size[NETWORKING_ATTR_SIZE];
};

#endif /* NETWORKING_TYPES_H */
