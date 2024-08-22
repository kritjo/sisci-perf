#include <sisci_types.h>
#include <sisci_api.h>
#include <unistd.h>
#include "simple_interrupt.h"
#include "protocol.h"
#include "common_read_write_functions.h"
#include "initiator_main.h"


void run_experiment_interrupt(sci_desc_t sd, __attribute__((unused)) pid_t main_pid, sci_remote_data_interrupt_t order_interrupt, sci_local_data_interrupt_t delivery_interrupt) {
    order_t order;
    delivery_t delivery;
    unsigned int size;
    sci_remote_interrupt_t interrupt;

    init_timer(MEASURE_SECONDS);

    order.commandType = COMMAND_TYPE_CREATE;
    order.orderType = ORDER_TYPE_INTERRUPT;
    order.id = 0;

    SEOE(SCITriggerDataInterrupt, order_interrupt, &order, sizeof(order), NO_FLAGS);

    size = sizeof(delivery);

    SEOE(SCIWaitForDataInterrupt, delivery_interrupt, &delivery, &size, SCI_INFINITE_TIMEOUT, NO_FLAGS);

    if (delivery.commandType != COMMAND_TYPE_CREATE || delivery.deliveryType != ORDER_TYPE_INTERRUPT || delivery.status != STATUS_TYPE_SUCCESS) {
        fprintf(stderr, "Error: Unexpected delivery\n");
        exit(1);
    }

    SEOE(SCIConnectInterrupt, sd, &interrupt, delivery.nodeId, ADAPTER_NO, delivery.id, SCI_INFINITE_TIMEOUT, NO_FLAGS);

    readable_printf("Starting interrupt experiment for %d seconds\n", MEASURE_SECONDS);
    start_timer();
    trigger_interrupt(interrupt);
    readable_printf("    operations: %llu\n", operations);
    machine_printf("$%s;%d;%llu\n", "INT", 1, operations);

    SEOE(SCIDisconnectInterrupt, interrupt, NO_FLAGS);

    destroy_timer();

    order.commandType = COMMAND_TYPE_DESTROY;
    order.id = delivery.id;

    SEOE(SCITriggerDataInterrupt, order_interrupt, &order, sizeof(order), NO_FLAGS);

    size = sizeof(delivery);

    SEOE(SCIWaitForDataInterrupt, delivery_interrupt, &delivery, &size, SCI_INFINITE_TIMEOUT, NO_FLAGS);

    if (delivery.commandType != COMMAND_TYPE_DESTROY || delivery.deliveryType != ORDER_TYPE_INTERRUPT || delivery.status != STATUS_TYPE_SUCCESS) {
        fprintf(stderr, "Error: Unexpected delivery\n");
        exit(1);
    }
}