#include <sisci_api.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include "scale_out_pio.h"
#include "sisci_glob_defs.h"
#include "protocol.h"
#include "initiator_main.h"
#include "common_read_write_functions.h"

void run_scale_out_segment_experiment_pio(sci_desc_t sd, pid_t main_pid, uint32_t num_peers, sci_remote_data_interrupt_t *order_interrupts, sci_local_data_interrupt_t delivery_interrupt) {
    sci_remote_segment_t segment[MAX_PEERS];
    sci_map_t map[MAX_PEERS];
    order_t order;
    delivery_t delivery;
    unsigned int size;
    volatile void *data[MAX_PEERS];
    sci_error_t error;
    sci_sequence_t sequence[MAX_PEERS];
    void *local_data;

    init_timer(MEASURE_SECONDS);

    local_data = malloc(MAX_BROADCAST_SEGMENT_SIZE);
    if (local_data == NULL) {
        fprintf(stderr, "Failed to allocate memory for local data\n");
        kill(main_pid, SIGTERM);
    }

    for (uint32_t segments_this_round = 1; segments_this_round <= num_peers; segments_this_round++) {

        for (uint32_t i = 0; i < segments_this_round; i++) {
            order.commandType = COMMAND_TYPE_CREATE;
            order.orderType = ORDER_TYPE_SEGMENT;
            order.size = MAX_BROADCAST_SEGMENT_SIZE;
            order.id = 0;

            SEOE(SCITriggerDataInterrupt, order_interrupts[i], &order, sizeof(order), NO_FLAGS);

            size = sizeof(delivery);

            SEOE(SCIWaitForDataInterrupt, delivery_interrupt, &delivery, &size, SCI_INFINITE_TIMEOUT, NO_FLAGS);

            if (delivery.commandType != COMMAND_TYPE_CREATE || delivery.deliveryType != ORDER_TYPE_SEGMENT ||
                delivery.status != STATUS_TYPE_SUCCESS) {
                fprintf(stderr, "Received invalid command type %d\n", delivery.commandType);
                kill(main_pid, SIGTERM);
            }

            SEOE(SCIConnectSegment, sd, &segment[i], delivery.nodeId, delivery.id, ADAPTER_NO, NO_CALLBACK, NO_ARG,
                 SCI_INFINITE_TIMEOUT, NO_FLAGS);

            data[i] = SCIMapRemoteSegment(segment[i], &map[i], NO_OFFSET, MAX_BROADCAST_SEGMENT_SIZE, NO_SUGGESTED_ADDRESS, NO_FLAGS,
                                          &error);
            if (error != SCI_ERR_OK) {
                fprintf(stderr, "Failed to map segment: %d\n", error);
                kill(main_pid, SIGTERM);
            }

            SEOE(SCICreateMapSequence, map[i], &sequence[i], NO_FLAGS);
            SEOE(SCIStartSequence, sequence[i], NO_FLAGS);
        }

        readable_printf("Starting PIO write for %d seconds with %d segments on different peers\n", MEASURE_SECONDS, segments_this_round);
        start_timer();
        write_pio_byte(data, MAX_BROADCAST_SEGMENT_SIZE, segments_this_round, NO_SEQUENCE, PIO_FLAG_NO_SEQ);
        readable_printf("    operations: %llu\n", operations);
        machine_printf("$PIO_WRITE_SCALE_OUT_%d;%d;%llu\n", segments_this_round, 1, operations);

        readable_printf("Starting PIO read for %d seconds with %d segments on different peers\n", MEASURE_SECONDS, segments_this_round);
        start_timer();
        read_pio_byte(data, MAX_BROADCAST_SEGMENT_SIZE, segments_this_round);
        readable_printf("    operations: %llu\n", operations);
        machine_printf("$PIO_READ_SCALE_OUT_%d;%d;%llu\n", segments_this_round, 1, operations);

        readable_printf("Starting PIO write %d bytes for %d seconds with %d segments on different peers\n", SEGMENT_SIZE, MEASURE_SECONDS, segments_this_round);
        start_timer();
        memcpy_write_pio(local_data, sequence, map, SEGMENT_SIZE, segments_this_round);
        readable_printf("    operations: %llu\n", operations);
        machine_printf("$PIO_WRITE_SCALE_OUT_%d;%d;%llu\n", segments_this_round, SEGMENT_SIZE, operations);

        readable_printf("Starting PIO write %d bytes for %d seconds with %d segments on different peers\n", MAX_BROADCAST_SEGMENT_SIZE, MEASURE_SECONDS, segments_this_round);
        start_timer();
        memcpy_write_pio(local_data, sequence, map, MAX_BROADCAST_SEGMENT_SIZE, segments_this_round);
        readable_printf("    operations: %llu\n", operations);
        machine_printf("$PIO_WRITE_SCALE_OUT_%d;%d;%llu\n", segments_this_round, MAX_BROADCAST_SEGMENT_SIZE, operations);

        for (uint32_t i = 0; i < segments_this_round; i++) {
            SEOE(SCIRemoveSequence, sequence[i], NO_FLAGS);

            SEOE(SCIUnmapSegment, map[i], NO_FLAGS);

            SEOE(SCIDisconnectSegment, segment[i], NO_FLAGS);

            order.commandType = COMMAND_TYPE_DESTROY;
            order.id = SCIGetRemoteSegmentId(segment[i]);

            SEOE(SCITriggerDataInterrupt, order_interrupts[i], &order, sizeof(order), NO_FLAGS);

            size = sizeof(delivery);

            SEOE(SCIWaitForDataInterrupt, delivery_interrupt, &delivery, &size, SCI_INFINITE_TIMEOUT, NO_FLAGS);

            if (delivery.commandType != COMMAND_TYPE_DESTROY || delivery.deliveryType != ORDER_TYPE_SEGMENT ||
                delivery.status != STATUS_TYPE_SUCCESS) {
                fprintf(stderr, "Received invalid command type %d\n", delivery.commandType);
                kill(main_pid, SIGTERM);
            }
        }
    }

    free(local_data);

    destroy_timer();
}