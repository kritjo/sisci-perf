#include <sisci_types.h>
#include <sys/types.h>

#include "broadcast_pio.h"
#include "initiator_main.h"
#include "protocol.h"
#include "common_read_write_functions.h"

void run_broadcast_dma_experiment(sci_desc_t sd, pid_t main_pid, uint32_t num_peers, sci_remote_data_interrupt_t *order_interrupts, sci_local_data_interrupt_t delivery_interrupt) {
    sci_remote_segment_t segment;
    sci_local_segment_t local_segment;
    sci_map_t local_map;
    volatile char *local_data;
    order_t order;
    delivery_t delivery;
    unsigned int size;
    sci_error_t error;
    sci_dma_queue_t dma_queue;

    init_timer(MEASURE_SECONDS);

    SEOE(SCICreateSegment, sd, &local_segment, BROADCAST_GROUP_ID, SEGMENT_SIZE, NO_CALLBACK, NO_ARG, NO_FLAGS);
    SEOE(SCIPrepareSegment, local_segment, ADAPTER_NO, SCI_FLAG_DMA_SOURCE_ONLY);

    local_data = (typeof(local_data)) SCIMapLocalSegment(local_segment, &local_map, NO_OFFSET, SEGMENT_SIZE, NO_SUGGESTED_ADDRESS, NO_FLAGS, &error);
    if (error != SCI_ERR_OK) {
        fprintf(stderr, "Error mapping local segment: %d\n", error);
        kill(main_pid, SIGTERM);
    }

    for (unsigned int i = 0; i < SEGMENT_SIZE / sizeof(char); i++) {
        local_data[i] = (char) i;
    }

    SEOE(SCICreateDMAQueue, sd, &dma_queue, ADAPTER_NO, DMA_QUEUE_MAX_ENTRIES, NO_FLAGS);

    for (uint32_t i = 0; i < num_peers; i++) {
        order.commandType = COMMAND_TYPE_CREATE;
        order.orderType = ORDER_TYPE_GLOBAL_DMA_BROADCAST_SEGMENT;
        order.size = SEGMENT_SIZE;
        order.id = BROADCAST_GROUP_ID;

        SEOE(SCITriggerDataInterrupt, order_interrupts[i], &order, sizeof(order), NO_FLAGS);

        size = sizeof(delivery);

        SEOE(SCIWaitForDataInterrupt, delivery_interrupt, &delivery, &size, SCI_INFINITE_TIMEOUT, NO_FLAGS);

        if (delivery.commandType != COMMAND_TYPE_CREATE || delivery.deliveryType != ORDER_TYPE_GLOBAL_DMA_BROADCAST_SEGMENT ||
            delivery.status != STATUS_TYPE_SUCCESS) {
            fprintf(stderr, "Received invalid command type %d\n", delivery.commandType);
            kill(main_pid, SIGTERM);
        }

        SEOE(SCIConnectSegment, sd, &segment, DIS_BROADCAST_NODEID_GROUP_ALL, BROADCAST_GROUP_ID, ADAPTER_NO, NO_CALLBACK, NO_ARG,
             SCI_INFINITE_TIMEOUT, SCI_FLAG_BROADCAST);

        block_for_dma(dma_queue);

        readable_printf("Starting DMA write one byte for %d seconds\n", MEASURE_SECONDS);
        start_timer();
        write_dma_broadcast(local_segment, segment, dma_queue, 1);
        readable_printf("    operations: %llu\n", operations);
        machine_printf("$%s;%d;%llu\n", "DMA_BROADCAST", 1, operations);

        SEOE(SCIDisconnectSegment, segment, NO_FLAGS);
    }

    SEOE(SCIRemoveDMAQueue, dma_queue, NO_FLAGS);

    for (uint32_t i = 0; i < num_peers; i++) {
        order.commandType = COMMAND_TYPE_DESTROY;
        order.id = BROADCAST_GROUP_ID;

        SEOE(SCITriggerDataInterrupt, order_interrupts[i], &order, sizeof(order), NO_FLAGS);

        size = sizeof(delivery);

        SEOE(SCIWaitForDataInterrupt, delivery_interrupt, &delivery, &size, SCI_INFINITE_TIMEOUT, NO_FLAGS);

        if (delivery.commandType != COMMAND_TYPE_DESTROY || delivery.deliveryType != ORDER_TYPE_GLOBAL_DMA_BROADCAST_SEGMENT ||
            delivery.status != STATUS_TYPE_SUCCESS) {
            fprintf(stderr, "Received invalid command type %d\n", delivery.commandType);
            kill(main_pid, SIGTERM);
        }
    }

    destroy_timer();
}