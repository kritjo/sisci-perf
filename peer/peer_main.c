#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include "sisci_glob_defs.h"
#include "sisci_api.h"

void print_usage(char *argv[]) {
    fprintf(stderr, "Usage: %s <initiator_id>\n", argv[0]);
}

int main(int argc, char *argv[]) {
    unsigned int initiator_id;
    long long_tmp;
    char *endptr;

    sci_desc_t sd;
    unsigned int order_interrupt_no;
    sci_local_data_interrupt_t order_interrupt;
    sci_remote_data_interrupt_t delivery_interrupt;
    sci_error_t error;

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
        SCIConnectDataInterrupt( sd, &delivery_interrupt, initiator_id, ADAPTER_NO, order_interrupt_no,
             SCI_INFINITE_TIMEOUT, NO_FLAGS, &error);
    } while (error != SCI_ERR_OK);

    SEOE(SCIRemoveDataInterrupt, order_interrupt, NO_FLAGS);

    SEOE(SCIClose, sd, NO_FLAGS);
    SCITerminate();
}