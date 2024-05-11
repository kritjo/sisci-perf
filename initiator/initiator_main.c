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
#include "simple_pio.h"
#include "scale_up_pio.h"
#include "simple_dma.h"
#include "scale_out_pio.h"
#include "initiator_main.h"
#include "var_size_pio.h"
#include "var_size_dma.h"

static pid_t main_pid;

uint32_t num_peers;
static uint32_t order_init_interrupts_received = 0;

static sci_remote_data_interrupt_t *order_interrupts;
static sci_desc_t sd;
static sci_local_data_interrupt_t delivery_interrupt;


static void print_usage(char *argv[]) {
    printf("Usage: ./%s <number of peers> <peer id> <peer id> <peer id> ...\n", argv[0]);
}

int main(int argc , char *argv[]) {
    long long_tmp;
    char *endptr;

    unsigned int delivery_interrupt_no = DELIVERY_INTERRUPT_NO;
    unsigned int *peer_ids;
    unsigned int length;

    main_pid = getpid();

    delivery_t delivery;

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

    if (num_peers > MAX_PEERS) {
        fprintf(stderr, "Number of peers must be less than or equal to %d\n", MAX_PEERS);
        exit(EXIT_FAILURE);
    }

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
         NO_CALLBACK,
         NO_ARG,
         SCI_FLAG_FIXED_INTNO);

    while (order_init_interrupts_received < num_peers) {
        length = sizeof(delivery);
        SEOE(SCIWaitForDataInterrupt, delivery_interrupt, &delivery, &length, SCI_INFINITE_TIMEOUT, NO_FLAGS);

        if (delivery.commandType != COMMAND_TYPE_CREATE && delivery.deliveryType != ORDER_TYPE_DATA_INTERRUPT) {
            fprintf(stderr, "Received invalid command type %d\n", delivery.commandType);
            kill(main_pid, SIGTERM);
        }

        SEOE(SCIConnectDataInterrupt, sd, &order_interrupts[order_init_interrupts_received++], delivery.nodeId, ADAPTER_NO, delivery.id, SCI_INFINITE_TIMEOUT, NO_FLAGS);
    }


    printf("##################### PIO EXPERIMENTS #####################\n");
    run_single_segment_experiment_pio(sd, main_pid, order_interrupts[0], delivery_interrupt);

    printf("##################### DMA EXPERIMENTS #####################\n");
    run_single_segment_experiment_dma(sd, main_pid, order_interrupts[0], delivery_interrupt);

    printf("################### SCALE-UP EXPERIMENTS ##################\n");
    run_scale_up_segment_experiment_pio(sd, main_pid, order_interrupts[0], delivery_interrupt);

    printf("################## SCALE-OUT EXPERIMENTS ##################\n");
    run_scale_out_segment_experiment_pio(sd, main_pid, num_peers, order_interrupts, delivery_interrupt);

    printf("################# VAR-SIZE PIO EXPERIMENTS #################\n");
    run_var_size_experiment_pio(sd, main_pid, order_interrupts[0], delivery_interrupt);

    printf("################# VAR-SIZE DMA EXPERIMENTS #################\n");
    run_var_size_experiment_dma(sd, main_pid, order_interrupts[0], delivery_interrupt);

    printf("##################### EXPERIMENTS END #####################\n");

    SEOE(SCIRemoveDataInterrupt, delivery_interrupt, NO_FLAGS);

    for (uint32_t i = 0; i < order_init_interrupts_received; i++) {
        SEOE(SCIDisconnectDataInterrupt, order_interrupts[i], NO_FLAGS);
    }

    SEOE(SCIClose, sd, NO_FLAGS);
    SCITerminate();

    free(peer_ids);
    free(order_interrupts);
}
