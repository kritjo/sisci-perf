#include <sisci_api.h>
#include <sys/time.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include "scale_up_pio.h"
#include "sisci_glob_defs.h"
#include "protocol.h"


static volatile sig_atomic_t timer_expired = 0;
static unsigned long long operations = 0;

static void timer_handler(__attribute__((unused)) int sig) {
    printf("    operations: %llu\n", operations);
    timer_expired = 1;
}

static void write_pio(volatile char *data[], uint32_t num_segments) {
    while (!timer_expired) {
        for (uint32_t i = 0; i < SEGMENT_SIZE; i++) {
            data[i % num_segments][i] = 0x01;
            operations++;
        }
    }
}

static void read_pio(volatile char *data[], uint32_t num_segments, pid_t main_pid) {
    while (!timer_expired) {
        for (uint32_t i = 0; i < SEGMENT_SIZE; i++) {
            if (data[i % num_segments][i] != 0x01) {
                fprintf(stderr, "Data mismatch at index %d: %d\n", i, data[i % num_segments][i]);
                kill(main_pid, SIGTERM);
            }
            operations++;
        }
    }
}

void run_scale_up_segment_experiment_pio(sci_desc_t sd, pid_t main_pid, sci_remote_data_interrupt_t order_interrupt, sci_local_data_interrupt_t delivery_interrupt) {
    // Order one segment from one peer
    sci_remote_segment_t segment[MAX_SEGMENTS];
    sci_map_t map[MAX_SEGMENTS];
    order_t order;
    delivery_t delivery;
    unsigned int size;
    volatile char *data[MAX_SEGMENTS];
    sci_error_t error;
    struct sigaction sa = {0};
    struct itimerval timer = {0};
    unsigned int segment_number;

    for (uint32_t i = 0; i < MAX_SEGMENTS; i++) {
        order.commandType = COMMAND_TYPE_CREATE;
        order.orderType = ORDER_TYPE_SEGMENT;
        order.size = SEGMENT_SIZE;

        SEOE(SCITriggerDataInterrupt, order_interrupt, &order, sizeof(order), NO_FLAGS);

        size = sizeof(delivery);

        SEOE(SCIWaitForDataInterrupt, delivery_interrupt, &delivery, &size, SCI_INFINITE_TIMEOUT, NO_FLAGS);

        if (delivery.commandType != COMMAND_TYPE_CREATE || delivery.deliveryType != ORDER_TYPE_SEGMENT ||
            delivery.status != STATUS_TYPE_SUCCESS) {
            fprintf(stderr, "Received invalid command type %d\n", delivery.commandType);
            kill(main_pid, SIGTERM);
        }

        SEOE(SCIConnectSegment, sd, &segment[i], delivery.nodeId, delivery.id, ADAPTER_NO, NO_CALLBACK, NO_ARG,
             SCI_INFINITE_TIMEOUT, NO_FLAGS);

        data[i] = SCIMapRemoteSegment(segment[i], &map[i], NO_OFFSET, SEGMENT_SIZE, NO_SUGGESTED_ADDRESS, NO_FLAGS, &error);
        if (error != SCI_ERR_OK) {
            fprintf(stderr, "Failed to map segment: %d\n", error);
            kill(main_pid, SIGTERM);
        }
    }

    sa.sa_handler = &timer_handler;
    sigaction(SIGALRM, &sa, NULL);

    timer.it_value.tv_sec = MEASURE_SECONDS;

    for (uint32_t i = 0; i < MAX_SEGMENTS; i++) {
        segment_number = i + 1;
        printf("Starting PIO write for %d seconds with %d segments on same peer\n", MEASURE_SECONDS, segment_number);
        timer_expired = 0;
        operations = 0;
        setitimer(ITIMER_REAL, &timer, NULL);
        write_pio(data, segment_number);

        printf("Starting PIO read for %d seconds with %d segments on same peer\n", MEASURE_SECONDS, segment_number);
        timer_expired = 0;
        operations = 0;
        setitimer(ITIMER_REAL, &timer, NULL);
        read_pio(data, segment_number, main_pid);
    }

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_DFL;
    if (sigaction(SIGALRM, &sa, NULL) == -1) {
        perror("Failed to reset signal handler");
        kill(main_pid, SIGTERM);
    }

    for (uint32_t i = 0; i < MAX_SEGMENTS; i++) {
        SEOE(SCIUnmapSegment, map[i], NO_FLAGS);

        SEOE(SCIDisconnectSegment, segment[i], NO_FLAGS);
    }

    order.commandType = COMMAND_TYPE_DESTROY;

    SEOE(SCITriggerDataInterrupt, order_interrupt, &order, sizeof(order), NO_FLAGS);

    size = sizeof(delivery);

    SEOE(SCIWaitForDataInterrupt, delivery_interrupt, &delivery, &size, SCI_INFINITE_TIMEOUT, NO_FLAGS);

    if (delivery.commandType != COMMAND_TYPE_DESTROY || delivery.deliveryType != ORDER_TYPE_SEGMENT || delivery.status != STATUS_TYPE_SUCCESS) {
        fprintf(stderr, "Received invalid command type %d\n", delivery.commandType);
        kill(main_pid, SIGTERM);
    }
}