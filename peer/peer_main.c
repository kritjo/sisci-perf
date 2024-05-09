#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <signal.h>
#include "sisci_glob_defs.h"
#include "sisci_api.h"
#include "protocol.h"
#include "print_node_id.h"
#include "block_for_termination_signal.h"

static pid_t main_pid;

static sci_desc_t sd;
static sci_remote_data_interrupt_t delivery_interrupt;

void print_usage(char *argv[]) {
    fprintf(stderr, "Usage: %s <initiator_id>\n", argv[0]);
}

static sci_local_segment_t *ordered_segments;
static unsigned int ordered_segments_count;

static sci_local_data_interrupt_t *ordered_data_interrupts;
static unsigned int ordered_data_interrupts_count;

static sci_local_interrupt_t *ordered_interrupts;
static unsigned int ordered_interrupts_count;

static void delivery_notification(delivery_status_t status, unsigned int commandType, unsigned int deliveryType, unsigned int id) {
    delivery_t delivery;

    delivery.status = status;
    delivery.commandType = commandType;
    delivery.deliveryType = deliveryType;
    delivery.id = id;
    delivery.nodeId = get_node_id();

    SEOE(SCITriggerDataInterrupt, delivery_interrupt, &delivery, sizeof(delivery), NO_FLAGS);
}

static sci_callback_action_t order_callback(__attribute__((unused)) void *_arg,
                                            __attribute__((unused)) sci_local_data_interrupt_t _interrupt,
                                            void *data,
                                            unsigned int length,
                                            sci_error_t status) {
    unsigned int id = 0;
    delivery_t delivery;


    if (status != SCI_ERR_OK) {
        fprintf(stderr, "Received error SCI status from delivery: %s\n", SCIGetErrorString(status));
        kill(main_pid, SIGTERM);
    }

    if (length != sizeof(order_t)) {
        fprintf(stderr, "Received invalid length %d from delivery\n", length);
        kill(main_pid, SIGTERM);
    }

    order_t *order = (order_t *) data;

    if (order->commandType == COMMAND_TYPE_CREATE) {
        switch (order->orderType) {
            case ORDER_TYPE_SEGMENT:
                ordered_segments = (typeof(ordered_segments)) reallocarray(ordered_segments, ordered_segments_count +1, sizeof(*ordered_segments));  // NOLINT(*-sizeof-expression)
                if (ordered_segments == NULL) {
                    perror("Failed to allocate memory for ordered segments");
                    kill(main_pid, SIGTERM);
                }

                SEOE(SCICreateSegment, sd, &ordered_segments[ordered_segments_count], id, order->size, NO_CALLBACK, NO_ARG, SCI_FLAG_AUTO_ID);

                SEOE(SCIPrepareSegment, ordered_segments[ordered_segments_count], ADAPTER_NO, NO_FLAGS);

                SEOE(SCISetSegmentAvailable, ordered_segments[ordered_segments_count], ADAPTER_NO, NO_FLAGS);

                ordered_segments_count++;

                delivery_notification(STATUS_TYPE_SUCCESS, COMMAND_TYPE_CREATE, ORDER_TYPE_SEGMENT, id);
                break;
            case ORDER_TYPE_GLOBAL_DMA_SEGMENT:
                ordered_segments = (typeof(ordered_segments)) reallocarray(ordered_segments, ordered_segments_count +1, sizeof(*ordered_segments));  // NOLINT(*-sizeof-expression)
                if (ordered_segments == NULL) {
                    perror("Failed to allocate memory for ordered segments");
                    kill(main_pid, SIGTERM);
                }
                id = 0;

                SEOE(SCICreateSegment, sd, &ordered_segments[ordered_segments_count], id, order->size, NO_CALLBACK, NO_ARG, SCI_FLAG_AUTO_ID | SCI_FLAG_DMA_GLOBAL);

                SEOE(SCIPrepareSegment, ordered_segments[ordered_segments_count], ADAPTER_NO, NO_FLAGS);

                SEOE(SCISetSegmentAvailable, ordered_segments[ordered_segments_count], ADAPTER_NO, NO_FLAGS);
                
                ordered_segments_count++;

                delivery_notification(STATUS_TYPE_SUCCESS, COMMAND_TYPE_CREATE, ORDER_TYPE_GLOBAL_DMA_SEGMENT, id);
                break;
            case ORDER_TYPE_INTERRUPT:
                ordered_interrupts = (typeof(ordered_interrupts)) reallocarray(ordered_interrupts, ordered_interrupts_count +1, sizeof(*ordered_interrupts));  // NOLINT(*-sizeof-expression)
                if (ordered_interrupts == NULL) {
                    perror("Failed to allocate memory for ordered interrupts");
                    kill(main_pid, SIGTERM);
                }

                SEOE(SCICreateInterrupt, sd, &ordered_interrupts[ordered_interrupts_count], ADAPTER_NO, &id, NO_CALLBACK, NO_ARG, NO_FLAGS);

                ordered_interrupts_count++;

                delivery_notification(STATUS_TYPE_SUCCESS, COMMAND_TYPE_CREATE, ORDER_TYPE_INTERRUPT, id);
                break;
            case ORDER_TYPE_DATA_INTERRUPT:
                ordered_data_interrupts = (typeof(ordered_data_interrupts)) reallocarray(ordered_data_interrupts, ordered_data_interrupts_count +1, sizeof(*ordered_data_interrupts));  // NOLINT(*-sizeof-expression)
                if (ordered_data_interrupts == NULL) {
                    perror("Failed to allocate memory for ordered data interrupts");
                    kill(main_pid, SIGTERM);
                }

                SEOE(SCICreateDataInterrupt, sd, &ordered_data_interrupts[ordered_data_interrupts_count], ADAPTER_NO, &id, order_callback, NO_ARG, NO_FLAGS);

                ordered_data_interrupts_count++;

                delivery_notification(STATUS_TYPE_SUCCESS, COMMAND_TYPE_CREATE, ORDER_TYPE_DATA_INTERRUPT, id);
                break;
        }
    } else if (order->commandType == COMMAND_TYPE_DESTROY) {
        // todo: handle deletion
    } else {
        fprintf(stderr, "Received invalid command type %d from delivery\n", order->commandType);
        kill(main_pid, SIGTERM);
    }

    return SCI_CALLBACK_CONTINUE;
}

int main(int argc, char *argv[]) {
    printf("Starting peer\n");
    unsigned int initiator_id;
    long long_tmp;
    char *endptr;

    unsigned int order_interrupt_no;
    sci_local_data_interrupt_t order_interrupt;
    unsigned int delivery_interrupt_no = DELIVERY_INTERRUPT_NO;

    sci_error_t error;

    main_pid = getpid();

    if (argc != 2) {
        print_usage(argv);
        exit(EXIT_FAILURE);
    }

    long_tmp = strtol(argv[1], &endptr, 10);
    if (*argv[1] == '\0' || *endptr != '\0') {
        print_usage(argv);
        exit(EXIT_FAILURE);

    }

    if (long_tmp < 1 || long_tmp > UINT_MAX) {
        print_usage(argv);
        exit(EXIT_FAILURE);
    }

    initiator_id = (typeof(initiator_id)) long_tmp;

    SEOE(SCIInitialize, NO_FLAGS);
    SEOE(SCIOpen, &sd, NO_FLAGS);

    SEOE(SCICreateDataInterrupt, sd, &order_interrupt, ADAPTER_NO, &order_interrupt_no, order_callback, NO_ARG, SCI_FLAG_USE_CALLBACK);

    do {
        SCIConnectDataInterrupt(sd, &delivery_interrupt, initiator_id, ADAPTER_NO, delivery_interrupt_no,
                                SCI_INFINITE_TIMEOUT, NO_FLAGS, &error);
    } while (error != SCI_ERR_OK);

    delivery_t delivery;
    delivery.status = STATUS_TYPE_PROTOCOL;
    delivery.commandType = COMMAND_TYPE_CREATE;
    delivery.deliveryType = ORDER_TYPE_DATA_INTERRUPT;
    delivery.id = order_interrupt_no;
    delivery.nodeId = get_node_id();

    SEOE(SCITriggerDataInterrupt, delivery_interrupt, &delivery, sizeof(delivery), NO_FLAGS);

    block_for_termination_signal();

    SEOE(SCIRemoveDataInterrupt, order_interrupt, NO_FLAGS);

    SEOE(SCIClose, sd, NO_FLAGS);
    SCITerminate();
}