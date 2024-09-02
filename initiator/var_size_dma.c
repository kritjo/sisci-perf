#include <sisci_api.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "var_size_dma.h"
#include "sisci_glob_defs.h"
#include "protocol.h"
#include "common_read_write_functions.h"
#include "initiator_main.h"

void run_var_size_experiment_dma(sci_desc_t sd, pid_t main_pid, sci_remote_data_interrupt_t order_interrupt, sci_local_data_interrupt_t delivery_interrupt) {
    sci_remote_segment_t segment;
    order_t order;
    delivery_t delivery;
    sci_error_t error;
    unsigned int size;
    sci_local_segment_t local_segment;
    sci_map_t local_map;
    char *local_ptr;
    sci_dma_queue_t dma_queue;
    size_t transfer_size;

    init_timer(MEASURE_SECONDS);

    order.commandType = COMMAND_TYPE_CREATE;
    order.orderType = ORDER_TYPE_GLOBAL_DMA_SEGMENT;
    order.size = MAX_SEGMENT_SIZE;
    order.id = 0;

    SEOE(SCITriggerDataInterrupt, order_interrupt, &order, sizeof(order), NO_FLAGS);

    size = sizeof(delivery);

    SEOE(SCIWaitForDataInterrupt, delivery_interrupt, &delivery, &size, SCI_INFINITE_TIMEOUT, NO_FLAGS);

    if (delivery.commandType != COMMAND_TYPE_CREATE || delivery.deliveryType != ORDER_TYPE_GLOBAL_DMA_SEGMENT || delivery.status != STATUS_TYPE_SUCCESS) {
        fprintf(stderr, "Received invalid command type %d\n", delivery.commandType);
        kill(main_pid, SIGTERM);
    }

    SEOE(SCIConnectSegment, sd, &segment, delivery.nodeId, delivery.id, ADAPTER_NO, NO_CALLBACK, NO_ARG, SCI_INFINITE_TIMEOUT, NO_FLAGS);

    SEOE(SCICreateSegment, sd, &local_segment, 0, MAX_SEGMENT_SIZE, NO_CALLBACK, NO_ARG, SCI_FLAG_AUTO_ID);
    SEOE(SCIPrepareSegment, local_segment, ADAPTER_NO, NO_FLAGS);

    local_ptr = (typeof(local_ptr)) SCIMapLocalSegment(local_segment, &local_map, NO_OFFSET, MAX_SEGMENT_SIZE, NO_SUGGESTED_ADDRESS, NO_FLAGS, &error);
    if (error != SCI_ERR_OK) {
        fprintf(stderr, "Error mapping local segment: %d\n", error);
        kill(main_pid, SIGTERM);
    }

    for (unsigned int i = 0; i < MAX_SEGMENT_SIZE / sizeof(char); i++) {
        local_ptr[i] = (char) i;
    }

    SEOE(SCICreateDMAQueue, sd, &dma_queue, ADAPTER_NO, DMA_QUEUE_MAX_ENTRIES, NO_FLAGS);

    transfer_size = MIN_MEASURE_DMA_TRANSFER_SIZE;
    while (transfer_size <= MAX_MEASURE_DMA_TRANSFER_SIZE) {
        block_for_dma(dma_queue);
        readable_printf("Starting DMA write %zu bytes for %d seconds\n", transfer_size, MEASURE_SECONDS);
        start_timer();
        write_dma(local_segment, segment, dma_queue, transfer_size);
        readable_printf("    bytes: %llu\n", operations*transfer_size);
        machine_printf("$%s;%zu;%llu\n", "DMA_WRITE", transfer_size, operations);

        block_for_dma(dma_queue);
        readable_printf("Starting DMA read %zu bytes for %d seconds\n", transfer_size, MEASURE_SECONDS);
        start_timer();
        read_dma(local_segment, segment, dma_queue, transfer_size);
        readable_printf("    bytes: %llu\n", operations*transfer_size);
        machine_printf("$%s;%zu;%llu\n", "DMA_READ", transfer_size, operations);

        transfer_size *= 2;
    }

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
