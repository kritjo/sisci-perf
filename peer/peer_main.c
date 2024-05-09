#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <signal.h>
#include "sisci_glob_defs.h"
#include "sisci_api.h"
#include "protocol.h"
#include "print_node_id.h"

static volatile sig_atomic_t signal_received = 0;

void print_usage(char *argv[]) {
    fprintf(stderr, "Usage: %s <initiator_id>\n", argv[0]);
}

static void cleanup_signal_handler(int sig) {
    signal_received = sig;
}

int main(int argc, char *argv[]) {
    printf("Starting peer\n");
    unsigned int initiator_id;
    long long_tmp;
    char *endptr;

    struct sigaction sa;
    sigset_t mask, oldmask;

    sci_desc_t sd;
    unsigned int order_interrupt_no;
    sci_local_data_interrupt_t order_interrupt;
    unsigned int delivery_interrupt_no = DELIVERY_INTERRUPT_NO;
    sci_remote_data_interrupt_t delivery_interrupt;
    sci_error_t error;

    if (argc != 2) {
        print_usage(argv);
        exit(EXIT_FAILURE);
    }

    long_tmp = strtol(argv[1], &endptr, 10);
    if (*argv[1] == '\0' || *endptr != '\0') {
        print_usage(argv);
        exit(EXIT_FAILURE);

    }

    if (long_tmp < 1 || long_tmp > UINT_MAX) {
        print_usage(argv);
        exit(EXIT_FAILURE);
    }

    initiator_id = (typeof(initiator_id)) long_tmp;

    SEOE(SCIInitialize, NO_FLAGS);
    SEOE(SCIOpen, &sd, NO_FLAGS);

    SEOE(SCICreateDataInterrupt, sd, &order_interrupt, ADAPTER_NO, &order_interrupt_no, NO_CALLBACK, NO_ARG, NO_FLAGS);

    do {
        SCIConnectDataInterrupt(sd, &delivery_interrupt, initiator_id, ADAPTER_NO, delivery_interrupt_no,
                                SCI_INFINITE_TIMEOUT, NO_FLAGS, &error);
    } while (error != SCI_ERR_OK);

    delivery_t delivery;
    delivery.status = STATUS_TYPE_PROTOCOL;
    delivery.commandType = COMMAND_TYPE_CREATE;
    delivery.deliveryType = ORDER_TYPE_DATA_INTERRUPT;
    delivery.id = order_interrupt_no;
    delivery.nodeId = get_node_id();

    SEOE(SCITriggerDataInterrupt, delivery_interrupt, &delivery, sizeof(delivery), NO_FLAGS);

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

    SEOE(SCIRemoveDataInterrupt, order_interrupt, NO_FLAGS);

    SEOE(SCIClose, sd, NO_FLAGS);
    SCITerminate();
}