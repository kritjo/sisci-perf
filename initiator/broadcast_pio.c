#include <sisci_types.h>
#include <sys/types.h>

#include "broadcast_pio.h"
#include "initiator_main.h"
#include "protocol.h"
#include "common_read_write_functions.h"

void run_broadcast_pio_experiment(sci_desc_t sd, pid_t main_pid, uint32_t num_peers, sci_remote_data_interrupt_t *order_interrupts, sci_local_data_interrupt_t delivery_interrupt) {
    sci_remote_segment_t segment;
    sci_map_t map;
    volatile void *data;
    order_t order;
    delivery_t delivery;
    unsigned int size;
    sci_error_t error;
    sci_sequence_t sequence;
    void *local_data;

    init_timer(MEASURE_SECONDS);

    local_data = malloc(MAX_BROADCAST_SEGMENT_SIZE);
    if (local_data == NULL) {
        fprintf(stderr, "Failed to allocate memory for local data\n");
        kill(main_pid, SIGTERM);
    }

    for (uint32_t i = 0; i < num_peers; i++) {
        order.commandType = COMMAND_TYPE_CREATE;
        order.orderType = ORDER_TYPE_BROADCAST_SEGMENT;
        order.size = MAX_BROADCAST_SEGMENT_SIZE;
        order.id = BROADCAST_GROUP_ID;

        SEOE(SCITriggerDataInterrupt, order_interrupts[i], &order, sizeof(order), NO_FLAGS);

        size = sizeof(delivery);

        SEOE(SCIWaitForDataInterrupt, delivery_interrupt, &delivery, &size, SCI_INFINITE_TIMEOUT, NO_FLAGS);

        if (delivery.commandType != COMMAND_TYPE_CREATE || delivery.deliveryType != ORDER_TYPE_BROADCAST_SEGMENT ||
            delivery.status != STATUS_TYPE_SUCCESS) {
            fprintf(stderr, "Received invalid command type %d\n", delivery.commandType);
            kill(main_pid, SIGTERM);
        }

        SEOE(SCIConnectSegment, sd, &segment, DIS_BROADCAST_NODEID_GROUP_ALL, BROADCAST_GROUP_ID, ADAPTER_NO, NO_CALLBACK, NO_ARG,
             SCI_INFINITE_TIMEOUT, SCI_FLAG_BROADCAST);

        data = SCIMapRemoteSegment(segment, &map,NO_OFFSET,MAX_BROADCAST_SEGMENT_SIZE,NULL,SCI_FLAG_BROADCAST,&error);
        if (error != SCI_ERR_OK) {
            fprintf(stderr,"SCIMapRemoteSegment failed - Error code 0x%x\n",error);
            exit(EXIT_FAILURE);
        }

        readable_printf("Starting PIO broadcast write one byte for %d seconds\n", MEASURE_SECONDS);
        start_timer();
        write_pio_byte(&data, MAX_BROADCAST_SEGMENT_SIZE, 1, NO_SEQUENCE, PIO_FLAG_NO_SEQ);
        readable_printf("    operations: %llu\n", operations);
        machine_printf("$PIO_BROADCAST_%d;%d;%llu\n", i+1, 1, operations);

        SEOE(SCICreateMapSequence, map, &sequence, NO_FLAGS);
        SEOE(SCIStartSequence, sequence, NO_FLAGS);

        readable_printf("Starting PIO broadcast write %d bytes for %d seconds\n", SEGMENT_SIZE, MEASURE_SECONDS);
        start_timer();
        memcpy_write_pio(local_data, &sequence, &map, SEGMENT_SIZE, 1);
        readable_printf("    operations: %llu\n", operations);
        machine_printf("$PIO_BROADCAST_%d;%d;%llu\n", i+1, SEGMENT_SIZE, operations);

        readable_printf("Starting PIO broadcast write %d bytes for %d seconds\n", MAX_BROADCAST_SEGMENT_SIZE, MEASURE_SECONDS);
        start_timer();
        memcpy_write_pio(local_data, &sequence, &map, MAX_BROADCAST_SEGMENT_SIZE, 1);
        readable_printf("    operations: %llu\n", operations);
        machine_printf("$PIO_BROADCAST_%d;%d;%llu\n", i+1, MAX_BROADCAST_SEGMENT_SIZE, operations);

        SEOE(SCIRemoveSequence, sequence, NO_FLAGS);

        SEOE(SCIUnmapSegment, map, NO_FLAGS);

        SEOE(SCIDisconnectSegment, segment, NO_FLAGS);
    }

    for (uint32_t i = 0; i < num_peers; i++) {
        order.commandType = COMMAND_TYPE_DESTROY;
        order.id = BROADCAST_GROUP_ID;

        SEOE(SCITriggerDataInterrupt, order_interrupts[i], &order, sizeof(order), NO_FLAGS);

        size = sizeof(delivery);

        SEOE(SCIWaitForDataInterrupt, delivery_interrupt, &delivery, &size, SCI_INFINITE_TIMEOUT, NO_FLAGS);

        if (delivery.commandType != COMMAND_TYPE_DESTROY || delivery.deliveryType != ORDER_TYPE_BROADCAST_SEGMENT ||
            delivery.status != STATUS_TYPE_SUCCESS) {
            fprintf(stderr, "Received invalid command type %d\n", delivery.commandType);
            kill(main_pid, SIGTERM);
        }
    }

    free(local_data);

    destroy_timer();
}