#include <sisci_api.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include "simple_dma.h"
#include "protocol.h"
#include "sisci_glob_defs.h"
#include "timer_controlled_variable.h"

__attribute__((unused)) static volatile sig_atomic_t *timer_expired;
__attribute__((unused)) static unsigned long long operations = 0;

static void write_dma(__attribute__((unused)) sci_local_segment_t local_segment,
                      __attribute__((unused)) sci_remote_segment_t remote_segment) {

}

static void read_dma(__attribute__((unused)) sci_local_segment_t local_segment, __attribute__((unused)) sci_remote_segment_t remote_segment) {

}

__attribute__((unused)) void run_single_segment_experiment_dma(sci_desc_t sd, pid_t main_pid, sci_remote_data_interrupt_t order_interrupt, sci_local_data_interrupt_t delivery_interrupt) {
    sci_remote_segment_t segment;
    order_t order;
    delivery_t delivery;
    sci_error_t error;
    unsigned int size;
    sci_local_segment_t local_segment;
    sci_map_t local_map;
    char *local_ptr;

    init_timer(MEASURE_SECONDS);
    timer_expired = get_timer_expired();

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
    start_timer();
    write_dma(local_segment, segment);

    printf("Starting DMA read for %d seconds\n", MEASURE_SECONDS);
    operations = 0;
    start_timer();
    read_dma(local_segment, segment);

    destroy_timer();

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
}