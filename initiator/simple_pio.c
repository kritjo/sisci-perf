#include <sisci_api.h>
#include <sys/time.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include "simple_pio.h"
#include "sisci_glob_defs.h"
#include "protocol.h"


static volatile sig_atomic_t timer_expired = 0;
static unsigned long long operations = 0;

void timer_handler(__attribute__((unused)) int sig) {
    printf("Timer expired\n");
    printf("Operations: %llu\n", operations);
    timer_expired = 1;
}

static void write_pio(volatile char *data) {
    while (!timer_expired) {
        for (uint32_t i = 0; i < SEGMENT_SIZE; i++) {
            data[i] = 0x01;
            operations++;
        }
    }
}

static void read_pio(volatile char *data, pid_t main_pid) {
    while (!timer_expired) {
        for (uint32_t i = 0; i < SEGMENT_SIZE; i++) {
            if (data[i] != 0x01) {
                fprintf(stderr, "Data mismatch at index %d: %d\n", i, data[i]);
                kill(main_pid, SIGTERM);
            }
            operations++;
        }
    }
}

void run_single_segment_experiment_pio(sci_desc_t sd, pid_t main_pid, sci_remote_data_interrupt_t order_interrupt, sci_local_data_interrupt_t delivery_interrupt) {
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
    order.size = SEGMENT_SIZE;

    SEOE(SCITriggerDataInterrupt, order_interrupt, &order, sizeof(order), NO_FLAGS);

    size = sizeof(delivery);

    SEOE(SCIWaitForDataInterrupt, delivery_interrupt, &delivery, &size, SCI_INFINITE_TIMEOUT, NO_FLAGS);

    if (delivery.commandType != COMMAND_TYPE_CREATE || delivery.deliveryType != ORDER_TYPE_SEGMENT || delivery.status != STATUS_TYPE_SUCCESS) {
        fprintf(stderr, "Received invalid command type %d\n", delivery.commandType);
        kill(main_pid, SIGTERM);
    }

    SEOE(SCIConnectSegment, sd, &segment, delivery.nodeId, delivery.id, ADAPTER_NO, NO_CALLBACK, NO_ARG, SCI_INFINITE_TIMEOUT, NO_FLAGS);

    data = SCIMapRemoteSegment(segment, &map, NO_OFFSET, SEGMENT_SIZE, NO_SUGGESTED_ADDRESS, NO_FLAGS, &error);
    if (error != SCI_ERR_OK) {
        fprintf(stderr, "Failed to map segment: %d\n", error);
        kill(main_pid, SIGTERM);
    }

    sa.sa_handler = &timer_handler;
    sigaction(SIGALRM, &sa, NULL);

    timer.it_value.tv_sec = MEASURE_SECONDS;

    printf("Starting PIO write for %d seconds\n", MEASURE_SECONDS);
    operations = 0;
    setitimer(ITIMER_REAL, &timer, NULL);
    write_pio(data);

    timer_expired = 0;

    printf("Starting PIO read for %d seconds\n", MEASURE_SECONDS);
    operations = 0;
    setitimer(ITIMER_REAL, &timer, NULL);
    read_pio(data, main_pid);

    SEOE(SCIUnmapSegment, map, NO_FLAGS);

    SEOE(SCIDisconnectSegment, segment, NO_FLAGS);

    order.commandType = COMMAND_TYPE_DESTROY;

    SEOE(SCITriggerDataInterrupt, order_interrupt, &order, sizeof(order), NO_FLAGS);

    size = sizeof(delivery);

    SEOE(SCIWaitForDataInterrupt, delivery_interrupt, &delivery, &size, SCI_INFINITE_TIMEOUT, NO_FLAGS);

    if (delivery.commandType != COMMAND_TYPE_DESTROY || delivery.deliveryType != ORDER_TYPE_SEGMENT || delivery.status != STATUS_TYPE_SUCCESS) {
        fprintf(stderr, "Received invalid command type %d\n", delivery.commandType);
        kill(main_pid, SIGTERM);
    }

    printf("Segment destroyed\n");
}