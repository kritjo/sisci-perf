#include <sisci_api.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "sisci_glob_defs.h"
#include "protocol.h"
#include "common_read_write_functions.h"
#include "initiator_main.h"
#include "var_size_segments.h"

void run_var_size_segments_experiment(sci_desc_t sd, pid_t main_pid, sci_remote_data_interrupt_t order_interrupt, sci_local_data_interrupt_t delivery_interrupt) {
    sci_remote_segment_t segment;
    sci_map_t map;
    order_t order;
    delivery_t delivery;
    unsigned int size;
    volatile void *data[1];
    sci_error_t error;
    sci_sequence_t sequence;
    sci_sequence_status_t sequence_status;

    init_timer(MEASURE_SECONDS);

    for (int segment_size = SEGMENT_SIZE; segment_size <= MAX_SEGMENT_SIZE; segment_size *= 2) {

        order.commandType = COMMAND_TYPE_CREATE;
        order.orderType = ORDER_TYPE_SEGMENT;
        order.size = segment_size;
        order.id = 0;

        SEOE(SCITriggerDataInterrupt, order_interrupt, &order, sizeof(order), NO_FLAGS);

        size = sizeof(delivery);

        SEOE(SCIWaitForDataInterrupt, delivery_interrupt, &delivery, &size, SCI_INFINITE_TIMEOUT, NO_FLAGS);

        if (delivery.commandType != COMMAND_TYPE_CREATE || delivery.deliveryType != ORDER_TYPE_SEGMENT ||
            delivery.status != STATUS_TYPE_SUCCESS) {
            fprintf(stderr, "Received invalid command type %d\n", delivery.commandType);
            kill(main_pid, SIGTERM);
        }

        SEOE(SCIConnectSegment, sd, &segment, delivery.nodeId, delivery.id, ADAPTER_NO, NO_CALLBACK, NO_ARG,
             SCI_INFINITE_TIMEOUT, NO_FLAGS);

        data[0] = SCIMapRemoteSegment(segment, &map, NO_OFFSET, segment_size, NO_SUGGESTED_ADDRESS, NO_FLAGS, &error);
        if (error != SCI_ERR_OK) {
            fprintf(stderr, "Failed to map segment: %d\n", error);
            kill(main_pid, SIGTERM);
        }

        SEOE(SCICreateMapSequence, map, &sequence, NO_FLAGS);
        do {
            sequence_status = SCIStartSequence(sequence, NO_FLAGS, &error);
            if (sequence_status != SCI_SEQ_OK && sequence_status != SCI_SEQ_PENDING) {
                fprintf(stderr, "Failed to start sequence, got illegal status: %d\n", sequence_status);
                exit(EXIT_FAILURE);
            }
        } while (sequence_status != SCI_SEQ_OK);

        readable_printf("Starting PIO write with flush for %d seconds in a %d sized segment\n", MEASURE_SECONDS, segment_size);
        start_timer();
        write_pio_byte(data, segment_size, 1, sequence, PIO_FLAG_FLUSH);
        readable_printf("    operations: %llu\n", operations);
        machine_printf("$PIO_WRITE_FLUSH_%d;%d;%llu\n", segment_size, 1, operations);

        SEOE(SCIUnmapSegment, map, NO_FLAGS);

        SEOE(SCIDisconnectSegment, segment, NO_FLAGS);

        order.commandType = COMMAND_TYPE_DESTROY;
        order.id = delivery.id;

        SEOE(SCITriggerDataInterrupt, order_interrupt, &order, sizeof(order), NO_FLAGS);

        size = sizeof(delivery);

        SEOE(SCIWaitForDataInterrupt, delivery_interrupt, &delivery, &size, SCI_INFINITE_TIMEOUT, NO_FLAGS);

        if (delivery.commandType != COMMAND_TYPE_DESTROY || delivery.deliveryType != ORDER_TYPE_SEGMENT ||
            delivery.status != STATUS_TYPE_SUCCESS) {
            fprintf(stderr, "Received invalid command type %d\n", delivery.commandType);
            kill(main_pid, SIGTERM);
        }
    }

    destroy_timer();
}