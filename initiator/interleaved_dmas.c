#include "interleaved_dmas.h"
#include "protocol.h"
#include "common_read_write_functions.h"
#include "initiator_main.h"

void run_single_segment_experiment_interleaved_dma(sci_desc_t sd, pid_t main_pid, uint32_t num_peers, sci_remote_data_interrupt_t *order_interrupts, sci_local_data_interrupt_t delivery_interrupt) {
    sci_remote_segment_t segments[NUM_CONCURRENT_SEGMENTS];
    order_t order;
    delivery_t delivery;
    sci_error_t error;
    unsigned int size;
    sci_local_segment_t local_segments[NUM_CONCURRENT_SEGMENTS];
    sci_map_t local_maps[NUM_CONCURRENT_SEGMENTS];
    char *local_ptr;
    sci_dma_queue_t dma_queues[NUM_CONCURRENT_SEGMENTS];

    init_timer(MEASURE_SECONDS);

    if (num_peers < NUM_CONCURRENT_SEGMENTS) {
        fprintf(stderr, "ERROR: num_peers < NUM_CONCURRENT_SEGMENTS\n");
        exit(EXIT_FAILURE);
    }

    for (uint32_t i = 0; i < NUM_CONCURRENT_SEGMENTS; i++) {
        order.commandType = COMMAND_TYPE_CREATE;
        order.orderType = ORDER_TYPE_GLOBAL_DMA_SEGMENT;
        order.size = SEGMENT_SIZE;
        order.id = 0;

        SEOE(SCITriggerDataInterrupt, order_interrupts[i], &order, sizeof(order), NO_FLAGS);

        size = sizeof(delivery);

        SEOE(SCIWaitForDataInterrupt, delivery_interrupt, &delivery, &size, SCI_INFINITE_TIMEOUT, NO_FLAGS);

        if (delivery.commandType != COMMAND_TYPE_CREATE || delivery.deliveryType != ORDER_TYPE_GLOBAL_DMA_SEGMENT || delivery.status != STATUS_TYPE_SUCCESS) {
            fprintf(stderr, "Received invalid command type %d\n", delivery.commandType);
            kill(main_pid, SIGTERM);
        }

        SEOE(SCIConnectSegment, sd, &segments[i], delivery.nodeId, delivery.id, ADAPTER_NO, NO_CALLBACK, NO_ARG,
             SCI_INFINITE_TIMEOUT, NO_FLAGS);

        SEOE(SCICreateSegment, sd, &local_segments[i], 0, SEGMENT_SIZE, NO_CALLBACK, NO_ARG, SCI_FLAG_AUTO_ID);
        SEOE(SCIPrepareSegment, local_segments[i], ADAPTER_NO, SCI_FLAG_DMA_SOURCE_ONLY);

        local_ptr = (typeof(local_ptr)) SCIMapLocalSegment(local_segments[i], &local_maps[i], NO_OFFSET, SEGMENT_SIZE, NO_SUGGESTED_ADDRESS, NO_FLAGS, &error);
        if (error != SCI_ERR_OK) {
            fprintf(stderr, "Error mapping local segment: %d\n", error);
            kill(main_pid, SIGTERM);
        }

        for (unsigned int j = 0; j < SEGMENT_SIZE / sizeof(char); j++) {
            local_ptr[j] = (char) j;
        }

        SEOE(SCICreateDMAQueue, sd, &dma_queues[i], ADAPTER_NO, DMA_QUEUE_MAX_ENTRIES, NO_FLAGS);
    }

    for (uint32_t i = 0; i < NUM_CONCURRENT_SEGMENTS; i++) {
        block_for_dma(dma_queues[i]);
    }
    readable_printf("Starting DMA write %d bytes for %d remotes for %d seconds\n", TRANSFER_SIZE, NUM_CONCURRENT_SEGMENTS, MEASURE_SECONDS);
    start_timer();
    write_dma(local_segments, segments, dma_queues, NUM_CONCURRENT_SEGMENTS, TRANSFER_SIZE);
    readable_printf("    operations: %llu\n", operations);
    machine_printf("$DMA_WRITE_CONCURRENT_%d;%d;%llu\n", NUM_CONCURRENT_SEGMENTS, TRANSFER_SIZE, operations);

    for (uint32_t i = 0; i < NUM_CONCURRENT_SEGMENTS; i++) {
        block_for_dma(dma_queues[i]);
    }
    readable_printf("Starting DMA read %d bytes for %d remotes for %d seconds\n", TRANSFER_SIZE, NUM_CONCURRENT_SEGMENTS, MEASURE_SECONDS);
    start_timer();
    read_dma(local_segments, segments, dma_queues, NUM_CONCURRENT_SEGMENTS, TRANSFER_SIZE);
    readable_printf("    operations: %llu\n", operations);
    machine_printf("$DMA_WRITE_CONCURRENT_%d;%d;%llu\n", NUM_CONCURRENT_SEGMENTS, TRANSFER_SIZE, operations);

    for (uint32_t i = 0; i < NUM_CONCURRENT_SEGMENTS; i++) {
        block_for_dma(dma_queues[i]);
    }

    destroy_timer();

    for (uint32_t i = 0; i < NUM_CONCURRENT_SEGMENTS; i++) {
        SEOE(SCIRemoveDMAQueue, dma_queues[i], NO_FLAGS);

        SEOE(SCIUnmapSegment, local_maps[i], NO_FLAGS);

        SEOE(SCIDisconnectSegment, segments[i], NO_FLAGS);

        SEOE(SCIRemoveSegment, local_segments[i], NO_FLAGS);

        order.commandType = COMMAND_TYPE_DESTROY;
        order.id = delivery.id;

        SEOE(SCITriggerDataInterrupt, order_interrupts[i], &order, sizeof(order), NO_FLAGS);

        size = sizeof(delivery);

        SEOE(SCIWaitForDataInterrupt, delivery_interrupt, &delivery, &size, SCI_INFINITE_TIMEOUT, NO_FLAGS);

        if (delivery.commandType != COMMAND_TYPE_DESTROY || delivery.deliveryType != ORDER_TYPE_GLOBAL_DMA_SEGMENT ||
            delivery.status != STATUS_TYPE_SUCCESS) {
            fprintf(stderr, "Received invalid command type %d\n", delivery.commandType);
            kill(main_pid, SIGTERM);
        }
    }
}