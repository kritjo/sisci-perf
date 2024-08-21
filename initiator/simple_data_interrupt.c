#include <sisci_types.h>
#include <sisci_api.h>
#include <unistd.h>
#include "simple_data_interrupt.h"
#include "protocol.h"
#include "common_read_write_functions.h"


void run_experiment_data_interrupt(sci_desc_t sd, __attribute__((unused)) pid_t main_pid, sci_remote_data_interrupt_t order_interrupt, sci_local_data_interrupt_t delivery_interrupt) {
    order_t order;
    delivery_t delivery;
    unsigned int size;
    sci_remote_data_interrupt_t interrupt;
    unsigned char data[DATA_INT_DATA_LENGTH];

    for (int i = 0; i < DATA_INT_DATA_LENGTH; i++) {
        data[i] = i;
    }

    init_timer(MEASURE_SECONDS);

    order.commandType = COMMAND_TYPE_CREATE;
    order.orderType = ORDER_TYPE_DATA_INTERRUPT;
    order.id = 0;

    SEOE(SCITriggerDataInterrupt, order_interrupt, &order, sizeof(order), NO_FLAGS);

    size = sizeof(delivery);

    SEOE(SCIWaitForDataInterrupt, delivery_interrupt, &delivery, &size, SCI_INFINITE_TIMEOUT, NO_FLAGS);

    if (delivery.commandType != COMMAND_TYPE_CREATE || delivery.deliveryType != ORDER_TYPE_DATA_INTERRUPT || delivery.status != STATUS_TYPE_SUCCESS) {
        fprintf(stderr, "Error: Unexpected delivery\n");
        exit(1);
    }

    SEOE(SCIConnectDataInterrupt, sd, &interrupt, delivery.nodeId, ADAPTER_NO, delivery.id, SCI_INFINITE_TIMEOUT, NO_FLAGS);

    printf("Starting trigger data interrupt with 1 byte of data for %d seconds\n", MEASURE_SECONDS);
    start_timer();
    trigger_data_interrupt(interrupt, data, 1);
    printf("    operations: %llu\n", operations);

    printf("Starting trigger data interrupt with %d bytes of data for %d seconds\n", DATA_INT_DATA_LENGTH, MEASURE_SECONDS);
    start_timer();
    trigger_data_interrupt(interrupt, data, DATA_INT_DATA_LENGTH);
    printf("    operations: %llu\n", operations);

    SEOE(SCIDisconnectDataInterrupt, interrupt, NO_FLAGS);

    destroy_timer();

    order.commandType = COMMAND_TYPE_DESTROY;
    order.id = delivery.id;

    SEOE(SCITriggerDataInterrupt, order_interrupt, &order, sizeof(order), NO_FLAGS);

    size = sizeof(delivery);

    SEOE(SCIWaitForDataInterrupt, delivery_interrupt, &delivery, &size, SCI_INFINITE_TIMEOUT, NO_FLAGS);

    if (delivery.commandType != COMMAND_TYPE_DESTROY || delivery.deliveryType != ORDER_TYPE_DATA_INTERRUPT || delivery.status != STATUS_TYPE_SUCCESS) {
        fprintf(stderr, "Error: Unexpected delivery\n");
        exit(1);
    }
}