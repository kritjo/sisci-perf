#include <signal.h>
#include <stddef.h>

#include "block_for_termination_signal.h"

static volatile sig_atomic_t signal_received = 0;

static void cleanup_signal_handler(int sig) {
    signal_received = sig;
}

void block_for_termination_signal() {
    struct sigaction sa;
    sigset_t mask, oldmask;

    sa.sa_handler = cleanup_signal_handler;
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGTSTP, &sa, NULL);

    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGTSTP);

    sigprocmask(SIG_BLOCK, &mask, &oldmask);

    while (!signal_received) {
        sigsuspend(&oldmask);
    }

    sigprocmask(SIG_UNBLOCK, &mask, NULL);
}