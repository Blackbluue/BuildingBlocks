#define _DEFAULT_SOURCE
#include "threaded_utils.h"
#include <ctype.h>
#include <string.h>

/* DATA */

#define ALPHABET_LEN 26

/* PRIVATE FUNCTIONS */

static size_t alpha_idx(char c) { return c - 'a'; }

/* PUBLIC FUNCTIONS */

void get_highest(char *string, char *character, uint16_t *count) {
    char letters[ALPHABET_LEN] = {0};
    char highest = string[0];
    for (size_t i = 0; i < strlen(string); i++) {
        char cur_letter = tolower((unsigned char)string[i]);
        size_t idx = alpha_idx(cur_letter);
        letters[idx]++;
        if (letters[idx] > letters[alpha_idx(highest)]) {
            highest = cur_letter;
        }
    }

    *character = highest;
    *count = letters[alpha_idx(highest)];
}
