#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
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
static sci_local_data_interrupt_t delivery_interrupt;


static void print_usage(char *argv[]) {
    printf("Usage: ./%s <number of peers> <peer id> <peer id> <peer id> ...\n", argv[0]);
}

static volatile sig_atomic_t timer_expired = 0;
static unsigned long long operations = 0;

void timer_handler(int sig) {
    printf("Timer expired\n");
    timer_expired = 1;
}

static void write_pio(volatile char *data) {
    while (!timer_expired) {
        for (uint32_t i = 0; i < 4096; i++) {
            data[i] = 0x01;
            operations++;
        }
    }
}

static void read_pio(volatile char *data) {
    while (!timer_expired) {
        for (uint32_t i = 0; i < 4096; i++) {
            if (data[i] != 0x01) {
                fprintf(stderr, "Data mismatch at index %d: %d\n", i, data[i]);
                kill(main_pid, SIGTERM);
            }
            operations++;
        }
    }
}

static void run_single_segment_experiment_pio() {
    // Order one segment from one peer
    sci_remote_segment_t segment;
    sci_map_t map;
    order_t order;
    delivery_t delivery;
    unsigned int size;
    volatile char *data;
    sci_error_t error;
    struct sigaction sa = {0};
    struct itimerval timer = {0};

    order.commandType = COMMAND_TYPE_CREATE;
    order.orderType = ORDER_TYPE_SEGMENT;
    order.size = 4096;

    SEOE(SCITriggerDataInterrupt, order_interrupts[0], &order, sizeof(order), NO_FLAGS);

    size = sizeof(delivery);

    SEOE(SCIWaitForDataInterrupt, delivery_interrupt, &delivery, &size, SCI_INFINITE_TIMEOUT, NO_FLAGS);

    if (delivery.commandType != COMMAND_TYPE_CREATE || delivery.deliveryType != ORDER_TYPE_SEGMENT || delivery.status != STATUS_TYPE_SUCCESS) {
        fprintf(stderr, "Received invalid command type %d\n", delivery.commandType);
        kill(main_pid, SIGTERM);
    }

    SEOE(SCIConnectSegment, sd, &segment, delivery.nodeId, delivery.id, ADAPTER_NO, NO_CALLBACK, NO_ARG, SCI_INFINITE_TIMEOUT, NO_FLAGS);

    data = SCIMapRemoteSegment(segment, &map, NO_OFFSET, 4096, NO_SUGGESTED_ADDRESS, NO_FLAGS, &error);
    if (error != SCI_ERR_OK) {
        fprintf(stderr, "Failed to map segment: %d\n", error);
        kill(main_pid, SIGTERM);
    }

    sa.sa_handler = &timer_handler;
    sigaction(SIGALRM, &sa, NULL);

    timer.it_value.tv_sec = 2;

    printf("Starting PIO write for 100 seconds\n");
    operations = 0;
    setitimer(ITIMER_REAL, &timer, NULL);
    write_pio(data);

    timer_expired = 0;

    printf("Starting PIO read for 100 seconds\n");
    operations = 0;
    setitimer(ITIMER_REAL, &timer, NULL);
    read_pio(data);

    SEOE(SCIUnmapSegment, map, NO_FLAGS);

    SEOE(SCIDisconnectSegment, segment, NO_FLAGS);

    order.commandType = COMMAND_TYPE_DESTROY;

    SEOE(SCITriggerDataInterrupt, order_interrupts[0], &order, sizeof(order), NO_FLAGS);

    size = sizeof(delivery);

    SEOE(SCIWaitForDataInterrupt, delivery_interrupt, &delivery, &size, SCI_INFINITE_TIMEOUT, NO_FLAGS);

    if (delivery.commandType != COMMAND_TYPE_DESTROY || delivery.deliveryType != ORDER_TYPE_SEGMENT || delivery.status != STATUS_TYPE_SUCCESS) {
        fprintf(stderr, "Received invalid command type %d\n", delivery.commandType);
        kill(main_pid, SIGTERM);
    }

    printf("Segment destroyed\n");
}

static void run_single_segment_experiment() {
    run_single_segment_experiment_pio();

    // Write 4 bytes to the peer for 100 seconds using DMA, measuring the number of operations executed

    // Read 4 bytes from the peer for 100 seconds using DMA, measuring the number of operations executed
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
        printf("Connected to peer %d\n", delivery.nodeId);
    }

    printf("All peers connected\n");

    run_single_segment_experiment();

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
