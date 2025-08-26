#define _XOPEN_SOURCE 500
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "common_read_write_functions.h"


struct sigaction sa = {0};
struct itimerval timer = {0};

volatile sig_atomic_t timer_expired = 0;

static void timer_handler(__attribute__((unused)) int sig) {
    timer_expired = 1;
}

void init_timer(time_t seconds) {
    sa.sa_handler = &timer_handler;
    if (sigaction(SIGALRM, &sa, NULL) == -1) {
        perror("Failed to set signal handler");
        exit(EXIT_FAILURE);
    }

    timer.it_value.tv_sec = seconds;
}

void start_timer() {
    timer_expired = 0;
    if (setitimer(ITIMER_REAL, &timer, NULL) == -1) {
        perror("Failed to start timer");
        exit(EXIT_FAILURE);
    }
}

void destroy_timer() {
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_DFL;
    if (sigaction(SIGALRM, &sa, NULL) == -1) {
        perror("Failed to reset signal handler");
        exit(EXIT_FAILURE);
    }
}