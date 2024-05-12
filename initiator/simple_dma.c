#include <sisci_api.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include "simple_dma.h"
#include "protocol.h"
#include "sisci_glob_defs.h"
#include "common_read_write_functions.h"

void run_single_segment_experiment_dma(sci_desc_t sd, pid_t main_pid, sci_remote_data_interrupt_t order_interrupt, sci_local_data_interrupt_t delivery_interrupt) {
    sci_remote_segment_t segment;
    order_t order;
    delivery_t delivery;
    sci_error_t error;
    unsigned int size;
    sci_local_segment_t local_segment;
    sci_map_t local_map;
    char *local_ptr;
    sci_dma_queue_t dma_queue;

    init_timer(MEASURE_SECONDS);

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
    SEOE(SCIPrepareSegment, local_segment, ADAPTER_NO, SCI_FLAG_DMA_SOURCE_ONLY);

    local_ptr = (typeof(local_ptr)) SCIMapLocalSegment(local_segment, &local_map, NO_OFFSET, SEGMENT_SIZE, NO_SUGGESTED_ADDRESS, NO_FLAGS, &error);
    if (error != SCI_ERR_OK) {
        fprintf(stderr, "Error mapping local segment: %d\n", error);
        kill(main_pid, SIGTERM);
    }

    for (unsigned int i = 0; i < SEGMENT_SIZE / sizeof(char); i++) {
        local_ptr[i] = (char) i;
    }

    SEOE(SCICreateDMAQueue, sd, &dma_queue, NO_CALLBACK, NO_ARG, NO_FLAGS);

    printf("Starting DMA write one byte for %d seconds\n", MEASURE_SECONDS);
    start_timer();
    write_dma(local_segment, segment, dma_queue, 1);
    printf("    operations: %llu\n", operations);

    printf("Starting DMA read one byte for %d seconds\n", MEASURE_SECONDS);
    start_timer();
    read_dma(local_segment, segment, dma_queue, 1);
    printf("    operations: %llu\n", operations);

    destroy_timer();

    SEOE(SCIRemoveDMAQueue, dma_queue, NO_FLAGS);

    SEOE(SCIUnmapSegment, local_map, NO_FLAGS);

    SEOE(SCIDisconnectSegment, segment, NO_FLAGS);

    SEOE(SCIRemoveSegment, local_segment, NO_FLAGS);

    order.commandType = COMMAND_TYPE_DESTROY;
    order.id = delivery.id;

    SEOE(SCITriggerDataInterrupt, order_interrupt, &order, sizeof(order), NO_FLAGS);

    size = sizeof(delivery);

    SEOE(SCIWaitForDataInterrupt, delivery_interrupt, &delivery, &size, SCI_INFINITE_TIMEOUT, NO_FLAGS);

    if (delivery.commandType != COMMAND_TYPE_DESTROY || delivery.deliveryType != ORDER_TYPE_GLOBAL_DMA_SEGMENT || delivery.status != STATUS_TYPE_SUCCESS) {
        fprintf(stderr, "Received invalid command type %d\n", delivery.commandType);
        kill(main_pid, SIGTERM);
    }
}