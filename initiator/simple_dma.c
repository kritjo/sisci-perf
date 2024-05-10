#include <sisci_api.h>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "simple_dma.h"
#include "protocol.h"
#include "sisci_glob_defs.h"

static volatile sig_atomic_t timer_expired = 0;
static unsigned long long operations = 0;

static void timer_handler(__attribute__((unused)) int sig) {
    printf("Timer expired\n");
    printf("Operations: %llu\n", operations);
    timer_expired = 1;
}

static void write_dma(sci_local_segment_t local_segment, sci_remote_segment_t remote_segment) {

}

static void read_dma(sci_local_segment_t local_segment, sci_remote_segment_t remote_segment) {

}

void run_single_segment_experiment_dma(sci_desc_t sd, pid_t main_pid, sci_remote_data_interrupt_t order_interrupt, sci_local_data_interrupt_t delivery_interrupt) {
    sci_remote_segment_t segment;
    order_t order;
    delivery_t delivery;
    sci_error_t error;
    unsigned int size;
    sci_local_segment_t local_segment;
    sci_map_t local_map;
    char *local_ptr;

    struct sigaction sa = {0};
    struct itimerval timer = {0};

    order.commandType = COMMAND_TYPE_CREATE;
    order.orderType = ORDER_TYPE_GLOBAL_DMA_SEGMENT;
    order.size = SEGMENT_SIZE;

    SEOE(SCITriggerDataInterrupt, order_interrupt, &order, sizeof(order), NO_FLAGS);

    size = sizeof(delivery);

    SEOE(SCIWaitForDataInterrupt, delivery_interrupt, &delivery, &size, SCI_INFINITE_TIMEOUT, NO_FLAGS);

    if (delivery.commandType != COMMAND_TYPE_CREATE || delivery.deliveryType != ORDER_TYPE_GLOBAL_DMA_SEGMENT || delivery.status != STATUS_TYPE_SUCCESS) {
        fprintf(stderr, "Received invalid command type %d\n", delivery.commandType);
        kill(main_pid, SIGTERM);
    }

    SEOE(SCIConnectSegment, sd, &segment, delivery.nodeId, delivery.id, ADAPTER_NO, NO_CALLBACK, NO_ARG, SCI_INFINITE_TIMEOUT, NO_FLAGS);

    sa.sa_handler = &timer_handler;
    sigaction(SIGALRM, &sa, NULL);

    timer.it_value.tv_sec = MEASURE_SECONDS;

    SEOE(SCICreateSegment, sd, &local_segment, 0, SEGMENT_SIZE, NO_CALLBACK, NO_ARG, SCI_FLAG_AUTO_ID);

    local_ptr = (typeof(local_ptr)) SCIMapLocalSegment(local_segment, &local_map, NO_OFFSET, SEGMENT_SIZE, NO_SUGGESTED_ADDRESS, NO_FLAGS, &error);
    if (error != SCI_ERR_OK) {
        fprintf(stderr, "Error mapping local segment: %d\n", error);
        kill(main_pid, SIGTERM);
    }

    for (unsigned int i = 0; i < SEGMENT_SIZE / sizeof(char); i++) {
        local_ptr[i] = (char) i;
    }

    printf("Starting DMA write for %d seconds\n", MEASURE_SECONDS);
    operations = 0;
    setitimer(ITIMER_REAL, &timer, NULL);
    write_dma(local_segment, segment);

    timer_expired = 0;

    printf("Starting DMA read for %d seconds\n", MEASURE_SECONDS);
    operations = 0;
    setitimer(ITIMER_REAL, &timer, NULL);
    read_dma(local_segment, segment);

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_DFL;
    if (sigaction(SIGALRM, &sa, NULL) == -1) {
        perror("Failed to reset signal handler");
        kill(main_pid, SIGTERM);
    }

    SEOE(SCIUnmapSegment, local_map, NO_FLAGS);

    SEOE(SCIDisconnectSegment, segment, NO_FLAGS);

    order.commandType = COMMAND_TYPE_DESTROY;

    SEOE(SCITriggerDataInterrupt, order_interrupt, &order, sizeof(order), NO_FLAGS);

    size = sizeof(delivery);

    SEOE(SCIWaitForDataInterrupt, delivery_interrupt, &delivery, &size, SCI_INFINITE_TIMEOUT, NO_FLAGS);

    if (delivery.commandType != COMMAND_TYPE_DESTROY || delivery.deliveryType != ORDER_TYPE_GLOBAL_DMA_SEGMENT || delivery.status != STATUS_TYPE_SUCCESS) {
        fprintf(stderr, "Received invalid command type %d\n", delivery.commandType);
        kill(main_pid, SIGTERM);
    }

    printf("Segment destroyed\n");

}