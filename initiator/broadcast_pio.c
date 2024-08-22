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

    init_timer(MEASURE_SECONDS);

    for (uint32_t i = 0; i < num_peers; i++) {
        order.commandType = COMMAND_TYPE_CREATE;
        order.orderType = ORDER_TYPE_BROADCAST_SEGMENT;
        order.size = SEGMENT_SIZE;
        order.id = BROADCAST_GROUP_ID;

        SEOE(SCITriggerDataInterrupt, order_interrupts[i], &order, sizeof(order), NO_FLAGS);

        size = sizeof(delivery);

        SEOE(SCIWaitForDataInterrupt, delivery_interrupt, &delivery, &size, SCI_INFINITE_TIMEOUT, NO_FLAGS);

        if (delivery.commandType != COMMAND_TYPE_CREATE || delivery.deliveryType != ORDER_TYPE_BROADCAST_SEGMENT ||
            delivery.status != STATUS_TYPE_SUCCESS) {
            fprintf(stderr, "Received invalid command type %d\n", delivery.commandType);
            kill(main_pid, SIGTERM);
        }
    }

    SEOE(SCIConnectSegment, sd, &segment, DIS_BROADCAST_NODEID_GROUP_ALL, BROADCAST_GROUP_ID, ADAPTER_NO, NO_CALLBACK, NO_ARG,
         SCI_INFINITE_TIMEOUT, SCI_FLAG_BROADCAST);

    data = SCIMapRemoteSegment(segment,&map,NO_OFFSET,SEGMENT_SIZE,NULL,SCI_FLAG_BROADCAST,&error);
    if (error != SCI_ERR_OK) {
        fprintf(stderr,"SCIMapRemoteSegment failed - Error code 0x%x\n",error);
        exit(EXIT_FAILURE);
    }

    printf("Starting DMA broadcast write one byte for %d seconds\n", MEASURE_SECONDS);
    start_timer();
    write_pio_byte(&data, SEGMENT_SIZE, 1, NO_SEQUENCE, PIO_FLAG_NO_SEQ);
    printf("    operations: %llu\n", operations);
    printf("$%s;%d;%llu\n", "PIO_BROADCAST", 1, operations);

    SEOE(SCIUnmapSegment, map, NO_FLAGS);

    SEOE(SCIDisconnectSegment, segment, NO_FLAGS);

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

    destroy_timer();
}