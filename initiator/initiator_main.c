#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <signal.h>
#include <unistd.h>
#include "protocol.h"
#include "sisci_api.h"
#include "sisci_types.h"
#include "sisci_glob_defs.h"
#include "block_for_termination_signal.h"

static pid_t main_pid;

uint32_t num_peers;
static uint32_t order_init_interrupts_received = 0;

static sci_remote_data_interrupt_t *order_interrupts;
static sci_desc_t sd;


static void print_usage(char *argv[]) {
    printf("Usage: ./%s <number of peers> <peer id> <peer id> <peer id> ...\n", argv[0]);
}



static sci_callback_action_t delivery_callback(__attribute__((unused)) void *_arg,
                                               __attribute__((unused)) sci_local_data_interrupt_t _interrupt,
                                               void *data,
                                               unsigned int length,
                                               sci_error_t status) {
    if (status != SCI_ERR_OK) {
        fprintf(stderr, "Received error SCI status from delivery: %s\n", SCIGetErrorString(status));
        kill(main_pid, SIGUSR1);
    }

    if (length != sizeof(delivery_t)) {
        fprintf(stderr, "Received invalid length %d from delivery\n", length);
        kill(main_pid, SIGUSR1);
    }

    delivery_t delivery = *(delivery_t *) data;

    switch (delivery.status) {
        case STATUS_TYPE_PROTOCOL:
            // Only known type is ORDER_TYPE_DATA_INTERRUPT for protocol messages
            if (delivery.commandType != COMMAND_TYPE_CREATE && delivery.deliveryType != ORDER_TYPE_DATA_INTERRUPT) {
                fprintf(stderr, "Received invalid command type %d\n", delivery.commandType);
                kill(main_pid, SIGTERM);
            }

            SEOE(SCIConnectDataInterrupt, sd, &order_interrupts[order_init_interrupts_received++], delivery.nodeId, ADAPTER_NO, delivery.id, SCI_INFINITE_TIMEOUT, NO_FLAGS);
            printf("Connected to peer %d\n", delivery.nodeId);

            break;
        case STATUS_TYPE_SUCCESS:
            break;
        case STATUS_TYPE_FAILURE:
            fprintf(stderr, "Received failure status\n");
            kill(main_pid, SIGTERM);
            break;
    }

    return SCI_CALLBACK_CONTINUE;
}

int main(int argc , char *argv[]) {
    long long_tmp;
    char *endptr;

    unsigned int delivery_interrupt_no = DELIVERY_INTERRUPT_NO;
    static sci_local_data_interrupt_t delivery_interrupt;
    static unsigned int *peer_ids;

    main_pid = getpid();

    if (argc < 3) {
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

    num_peers = (typeof(num_peers)) long_tmp;

    if (argc != (int) num_peers + 2) {
        print_usage(argv);
        exit(EXIT_FAILURE);
    }

    peer_ids = (typeof(peer_ids)) malloc(num_peers * sizeof(*peer_ids));

    for (uint32_t i = 0; i < num_peers; i++) {
        long_tmp = strtol(argv[i + 2], &endptr, 10);
        if (*argv[i + 2] == '\0' || *endptr != '\0') {
            print_usage(argv);
            exit(EXIT_FAILURE);
        }

        if (long_tmp < 0 || long_tmp > UINT_MAX) {
            print_usage(argv);
            exit(EXIT_FAILURE);
        }

        peer_ids[i] = (typeof(peer_ids[i])) long_tmp;
    }

    order_interrupts = (typeof(order_interrupts)) malloc(num_peers * sizeof(*order_interrupts)); // NOLINT(*-sizeof-expression) -- actually want the size of the type

    SEOE(SCIInitialize, NO_FLAGS);
    SEOE(SCIOpen, &sd, NO_FLAGS);

    SEOE(SCICreateDataInterrupt,
         sd,
         &delivery_interrupt,
         ADAPTER_NO,
         &delivery_interrupt_no,
         delivery_callback,
         NO_ARG,
         SCI_FLAG_FIXED_INTNO | SCI_FLAG_USE_CALLBACK);

    block_for_termination_signal();

    SEOE(SCIRemoveDataInterrupt, delivery_interrupt, NO_FLAGS);

    for (uint32_t i = 0; i < order_init_interrupts_received; i++) {
        SEOE(SCIDisconnectDataInterrupt, order_interrupts[i], NO_FLAGS);
    }

    SEOE(SCIClose, sd, NO_FLAGS);
    SCITerminate();

    free(peer_ids);
    free(order_interrupts);
}
