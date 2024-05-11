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

    printf("Starting PIO write for %d seconds\n", MEASURE_SECONDS);
    operations = 0;
    start_timer();
    write_pio_byte(data, SEGMENT_SIZE, 1);
    printf("    operations: %llu\n", operations);

    printf("Starting PIO read for %d seconds\n", MEASURE_SECONDS);
    operations = 0;
    start_timer();
    read_pio_byte(data, SEGMENT_SIZE, 1);
    printf("    operations: %llu\n", operations);

    SEOE(SCIUnmapSegment, map, NO_FLAGS);

    SEOE(SCIDisconnectSegment, segment, NO_FLAGS);

    SEOE(SCIConnectSegment, sd, &segment, delivery.nodeId, delivery.id, ADAPTER_NO, NO_CALLBACK, NO_ARG, SCI_INFINITE_TIMEOUT, NO_FLAGS);

    data[0] = SCIMapRemoteSegment(segment, &map, NO_OFFSET, SEGMENT_SIZE, NO_SUGGESTED_ADDRESS, SCI_FLAG_IO_MAP_IOSPACE, &error);
    if (error != SCI_ERR_OK) {
        fprintf(stderr, "Failed to map segment: %d\n", error);
        kill(main_pid, SIGTERM);
    }

    printf("Starting PIO write in io-space for %d seconds\n", MEASURE_SECONDS);
    operations = 0;
    start_timer();
    write_pio_byte(data, SEGMENT_SIZE, 1);
    printf("    operations: %llu\n", operations);

    printf("Starting PIO read in io-space for %d seconds\n", MEASURE_SECONDS);
    operations = 0;
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