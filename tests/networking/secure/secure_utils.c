#define _POSIX_C_SOURCE 200112L
#include "secure_utils.h"
#include <signal.h>
#include <unistd.h>

/* DATA */

/* PRIVATE FUNCTIONS */

static void exit_handler(int sig) { (void)sig; }

/* PUBLIC FUNCTIONS */

void allow_graceful_exit(void) {
    struct sigaction action;
    action.sa_handler = exit_handler;
    action.sa_flags = 0;
    sigemptyset(&action.sa_mask);
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGTERM, &action, NULL);
}
