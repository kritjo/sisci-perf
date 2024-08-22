#include <sisci_types.h>
#include <sisci_api.h>
#include <unistd.h>
#include <stdlib.h>
#include "ping_pong_pio.h"
#include "sisci_glob_defs.h"
#include "protocol.h"
#include "common_read_write_functions.h"


void run_ping_pong_experiment_pio(sci_desc_t sd, sci_remote_data_interrupt_t order_interrupt, sci_local_data_interrupt_t delivery_interrupt) {
    sci_local_segment_t local_segment;
    sci_map_t local_map;
    unsigned char *local_ptr;

    order_t order;
    delivery_t delivery;
    unsigned int size;

    sci_remote_segment_t remote_segment;
    sci_map_t remote_map;
    peer_ping_pong_segment_t *remote_ptr;
    sci_error_t error;
    sci_sequence_t sequence;

    // Allocate local segment
    SEOE(SCICreateSegment, sd, &local_segment, 0, SEGMENT_SIZE, NO_CALLBACK, NO_ARG, SCI_FLAG_AUTO_ID);
    SEOE(SCIPrepareSegment, local_segment, ADAPTER_NO, NO_FLAGS);
    SEOE(SCISetSegmentAvailable, local_segment, ADAPTER_NO, NO_FLAGS);

    // Map local segment
    local_ptr = (typeof(local_ptr)) SCIMapLocalSegment(local_segment, &local_map, 0, SEGMENT_SIZE, NULL, NO_FLAGS, &error);
    if (error != SCI_ERR_OK) {
        fprintf(stderr, "SCIMapLocalSegment failed: %s\n", SCIGetErrorString(error));
        exit(EXIT_FAILURE);
    }

    *local_ptr = 0;

    // Order a ping pong segment from peer
    order.orderType = ORDER_TYPE_PING_PONG_SEGMENT;
    order.commandType = COMMAND_TYPE_CREATE;
    order.size = SEGMENT_SIZE;
    order.id = 0;

    SEOE(SCITriggerDataInterrupt, order_interrupt, &order, sizeof(order), NO_FLAGS);

    size = sizeof(delivery);

    SEOE(SCIWaitForDataInterrupt, delivery_interrupt, &delivery, &size, SCI_INFINITE_TIMEOUT, NO_FLAGS);

    if (delivery.commandType != COMMAND_TYPE_CREATE || delivery.deliveryType != ORDER_TYPE_PING_PONG_SEGMENT || delivery.status != STATUS_TYPE_SUCCESS) {
        fprintf(stderr, "Received invalid command type %d\n", delivery.commandType);
        exit(EXIT_FAILURE);
    }

    // Connect to remote segment
    SEOE(SCIConnectSegment, sd, &remote_segment, delivery.nodeId, delivery.id, ADAPTER_NO, NO_CALLBACK, NO_ARG, SCI_INFINITE_TIMEOUT, NO_FLAGS);

    // Map remote segment
    remote_ptr = (typeof (remote_ptr)) SCIMapRemoteSegment(remote_segment, &remote_map, 0, SEGMENT_SIZE, NULL, NO_FLAGS, &error);
    if (error != SCI_ERR_OK) {
        fprintf(stderr, "SCIMapRemoteSegment failed: %s\n", SCIGetErrorString(error));
        exit(EXIT_FAILURE);
    }

    SEOE(SCICreateMapSequence, remote_map, &sequence, NO_FLAGS);
    SEOE(SCIStartSequence, sequence, NO_FLAGS);

    remote_ptr->initiator_ping_pong_segment_id = SCIGetLocalSegmentId(local_segment);
    remote_ptr->counter = 0;
    remote_ptr->initiator_ready = true;

    init_timer(MEASURE_SECONDS);

    printf("Starting PIO ping pong experiment for %d seconds\n", MEASURE_SECONDS);
    start_timer();
    ping_pong_pio(local_ptr, remote_ptr, sequence, local_map);
    printf("    operations: %llu\n", operations);
    printf("$%s;%d;%llu\n", "PIO_PINGPONG", 0, operations);


    destroy_timer();

    order.commandType = COMMAND_TYPE_DESTROY;
    order.id = delivery.id;

    SEOE(SCITriggerDataInterrupt, order_interrupt, &order, sizeof(order), NO_FLAGS);

    size = sizeof(delivery);

    SEOE(SCIWaitForDataInterrupt, delivery_interrupt, &delivery, &size, SCI_INFINITE_TIMEOUT, NO_FLAGS);

    if (delivery.commandType != COMMAND_TYPE_DESTROY || delivery.deliveryType != ORDER_TYPE_PING_PONG_SEGMENT ||
        delivery.status != STATUS_TYPE_SUCCESS) {
        fprintf(stderr, "Received invalid command type %d\n", delivery.commandType);
        exit(EXIT_FAILURE);
    }

    SEOE(SCIRemoveSequence, sequence, NO_FLAGS);
    SEOE(SCIUnmapSegment, remote_map, NO_FLAGS);
    SEOE(SCIDisconnectSegment, remote_segment, NO_FLAGS);
    SEOE(SCISetSegmentUnavailable, local_segment, ADAPTER_NO, NO_FLAGS);
    SEOE(SCIUnmapSegment, local_map, NO_FLAGS);
    SEOE(SCIRemoveSegment, local_segment, NO_FLAGS);
}