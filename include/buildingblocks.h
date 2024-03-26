#ifndef BUILDINGBLOCKS_H
#define BUILDINGBLOCKS_H

/* DATA */

enum query_cmds {
    QUERY_SIZE,
    QUERY_CAPACITY,
    QUERY_IS_EMPTY,
    QUERY_IS_FULL,
};

/* FUNCTIONS */

#ifdef DEBUG
#include <stdio.h>
#define DEBUG_PRINT(fmt, ...)                                                  \
    fprintf(stderr, "DEBUG: %s:%d:%s(): " fmt, __FILE__, __LINE__, __func__,   \
            ##__VA_ARGS__)
#else
#define DEBUG_PRINT(fmt, ...) /* Don't do anything in release builds */
#endif

/**
 * @brief Sets the error code.
 *
 * @param err The error code.
 * @param value The value to set.
 */
void set_err(int *err, int value);

#endif /* BUILDINGBLOCKS_H */
