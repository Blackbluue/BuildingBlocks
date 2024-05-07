#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>

/* DATA */

#define MAX_STR_LEN 61
#define NO_FLAGS 0
#define PORT_BASE 53468 // default port for counters
#define PORT_RANGE 5    // number of ports to try

enum {
    SVR_SUCCESS = 99, // server response flag: success
    SVR_FAILURE,      // server response flag: internal error
    SVR_INVALID,      // server response flag: invalid request
    RQU_REPEAT,       // client request flag: repeat character
    RQU_COUNT,        // client request flag: count characters
};

struct counter_packet {
    uint16_t count; // number of times character is repeated
    union {
        char character;               // single character to be repeated
        char string[MAX_STR_LEN + 1]; // null-terminated string
    };
};

/* FUNCTIONS */

/**
 * @brief Count the number of times a character is repeated in a string.
 *
 * @param string - The string to process.
 * @param character - The character that is repeated the most.
 * @param count - The number of times the character is repeated.
 */
void get_highest(char *string, char *character, uint16_t *count);

#endif /* UTILS_H */
