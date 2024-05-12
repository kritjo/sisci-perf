#include <sisci_api.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "simple_pio.h"
#include "sisci_glob_defs.h"
#include "protocol.h"
#include "common_read_write_functions.h"

void run_single_segment_experiment_pio(sci_desc_t sd, pid_t main_pid, sci_remote_data_interrupt_t order_interrupt, sci_local_data_interrupt_t delivery_interrupt) {
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

    data[0] = SCIMapRemoteSegment(segment, &map, NO_OFFSET, SEGMENT_SIZE, NO_SUGGESTED_ADDRESS, NO_FLAGS, &error);
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

    printf("Starting PIO write for %d seconds\n", MEASURE_SECONDS);
    start_timer();
    write_pio_byte(data, SEGMENT_SIZE, 1, NO_SEQUENCE, PIO_FLAG_NO_SEQ);
    printf("    operations: %llu\n", operations);

    printf("Starting PIO read for %d seconds\n", MEASURE_SECONDS);
    start_timer();
    read_pio_byte(data, SEGMENT_SIZE, 1);
    printf("    operations: %llu\n", operations);

    printf("Starting PIO write with flush for %d seconds\n", MEASURE_SECONDS);
    start_timer();
    write_pio_byte(data, SEGMENT_SIZE, 1, sequence, PIO_FLAG_FLUSH);
    printf("    operations: %llu\n", operations);

    sequence_status = SCICheckSequence(sequence, NO_FLAGS, &error);

    if (sequence_status != SCI_SEQ_OK) {
        fprintf(stderr, "Sequence failed: %d\n", error);
        exit(EXIT_FAILURE);
    }

    printf("Starting PIO write with sequence check for %d seconds\n", MEASURE_SECONDS);
    start_timer();
    write_pio_byte(data, SEGMENT_SIZE, 1, sequence, PIO_FLAG_CHK_SEQ);
    printf("    operations: %llu\n", operations);

    SEOE(SCIRemoveSequence, sequence, NO_FLAGS);

    SEOE(SCIUnmapSegment, map, NO_FLAGS);

    SEOE(SCIDisconnectSegment, segment, NO_FLAGS);

    sleep(1);

    SEOE(SCIConnectSegment, sd, &segment, delivery.nodeId, delivery.id, ADAPTER_NO, NO_CALLBACK, NO_ARG, SCI_INFINITE_TIMEOUT, NO_FLAGS);

    data[0] = SCIMapRemoteSegment(segment, &map, NO_OFFSET, SEGMENT_SIZE, NO_SUGGESTED_ADDRESS, SCI_FLAG_IO_MAP_IOSPACE, &error);
    if (error != SCI_ERR_OK) {
        fprintf(stderr, "Failed to map segment: %d\n", error);
        kill(main_pid, SIGTERM);
    }

    printf("Starting PIO write in io-space for %d seconds\n", MEASURE_SECONDS);
    start_timer();
    write_pio_byte(data, SEGMENT_SIZE, 1, NO_SEQUENCE, PIO_FLAG_NO_SEQ);
    printf("    operations: %llu\n", operations);

    printf("Starting PIO read in io-space for %d seconds\n", MEASURE_SECONDS);
    start_timer();
    read_pio_byte(data, SEGMENT_SIZE, 1);
    printf("    operations: %llu\n", operations);

    SEOE(SCIUnmapSegment, map, NO_FLAGS);

    destroy_timer();

    order.commandType = COMMAND_TYPE_DESTROY;
    order.id = delivery.id;

    SEOE(SCITriggerDataInterrupt, order_interrupt, &order, sizeof(order), NO_FLAGS);

    size = sizeof(delivery);

    SEOE(SCIWaitForDataInterrupt, delivery_interrupt, &delivery, &size, SCI_INFINITE_TIMEOUT, NO_FLAGS);

    if (delivery.commandType != COMMAND_TYPE_DESTROY || delivery.deliveryType != ORDER_TYPE_SEGMENT || delivery.status != STATUS_TYPE_SUCCESS) {
        fprintf(stderr, "Received invalid command type %d\n", delivery.commandType);
        kill(main_pid, SIGTERM);
    }
}