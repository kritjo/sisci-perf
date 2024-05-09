#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <signal.h>
#include "sisci_glob_defs.h"
#include "sisci_api.h"
#include "protocol.h"
#include "print_node_id.h"
#include "block_for_termination_signal.h"

static pid_t main_pid;

void print_usage(char *argv[]) {
    fprintf(stderr, "Usage: %s <initiator_id>\n", argv[0]);
}

static sci_callback_action_t order_callback(__attribute__((unused)) void *_arg,
                                            __attribute__((unused)) sci_local_data_interrupt_t _interrupt,
                                            void *data,
                                            unsigned int length,
                                            sci_error_t status) {
    if (status != SCI_ERR_OK) {
        fprintf(stderr, "Received error SCI status from delivery: %s\n", SCIGetErrorString(status));
        kill(main_pid, SIGTERM);
    }

    if (length != sizeof(order_t)) {
        fprintf(stderr, "Received invalid length %d from delivery\n", length);
        kill(main_pid, SIGTERM);
    }

    order_t *order = (order_t *) data;

    if (order->commandType == COMMAND_TYPE_CREATE) {
        // todo: handle creation
    } else if (order->commandType == COMMAND_TYPE_DESTROY) {
        // todo: handle deletion
    } else {
        fprintf(stderr, "Received invalid command type %d from delivery\n", order->commandType);
        kill(main_pid, SIGTERM);
    }

    return SCI_CALLBACK_CONTINUE;
}

int main(int argc, char *argv[]) {
    printf("Starting peer\n");
    unsigned int initiator_id;
    long long_tmp;
    char *endptr;

    sci_desc_t sd;
    unsigned int order_interrupt_no;
    sci_local_data_interrupt_t order_interrupt;
    unsigned int delivery_interrupt_no = DELIVERY_INTERRUPT_NO;
    sci_remote_data_interrupt_t delivery_interrupt;
    sci_error_t error;

    main_pid = getpid();

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

    SEOE(SCICreateDataInterrupt, sd, &order_interrupt, ADAPTER_NO, &order_interrupt_no, order_callback, NO_ARG, SCI_FLAG_USE_CALLBACK);

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

    block_for_termination_signal();

    SEOE(SCIRemoveDataInterrupt, order_interrupt, NO_FLAGS);

    SEOE(SCIClose, sd, NO_FLAGS);
    SCITerminate();
}