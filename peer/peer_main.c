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

static unsigned int *ordered_data_interrupt_nos;
static sci_local_data_interrupt_t *ordered_data_interrupts;
static unsigned int ordered_data_interrupts_count;

static unsigned int *ordered_interrupt_nos;
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
    sci_local_segment_t *tmp_ordered_segments;
    sci_local_segment_t *new_ordered_segments;
    sci_local_interrupt_t *tmp_ordered_interrupts;
    sci_local_interrupt_t *new_ordered_interrupts;
    unsigned int *tmp_ordered_interrupt_nos;
    unsigned int *new_ordered_interrupt_nos;
    sci_local_data_interrupt_t *tmp_ordered_data_interrupts;
    sci_local_data_interrupt_t *new_ordered_data_interrupts;
    unsigned int *tmp_ordered_data_interrupt_nos;
    unsigned int *new_ordered_data_interrupt_nos;

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
            case ORDER_TYPE_GLOBAL_DMA_SEGMENT:
                ordered_segments = (typeof(ordered_segments)) reallocarray(ordered_segments, ordered_segments_count +1, sizeof(*ordered_segments));  // NOLINT(*-sizeof-expression)
                if (ordered_segments == NULL) {
                    perror("Failed to allocate memory for ordered segments");
                    kill(main_pid, SIGTERM);
                }

                unsigned int create_flags = order->orderType == ORDER_TYPE_GLOBAL_DMA_SEGMENT ? SCI_FLAG_DMA_GLOBAL | SCI_FLAG_AUTO_ID : SCI_FLAG_AUTO_ID;

                SEOE(SCICreateSegment, sd, &ordered_segments[ordered_segments_count], id, order->size, NO_CALLBACK, NO_ARG, create_flags);

                SEOE(SCIPrepareSegment, ordered_segments[ordered_segments_count], ADAPTER_NO, NO_FLAGS);

                SEOE(SCISetSegmentAvailable, ordered_segments[ordered_segments_count], ADAPTER_NO, NO_FLAGS);

                delivery_notification(STATUS_TYPE_SUCCESS, COMMAND_TYPE_CREATE, order->orderType, SCIGetLocalSegmentId(ordered_segments[ordered_segments_count]));

                ordered_segments_count++;
                break;
            case ORDER_TYPE_INTERRUPT:
                ordered_interrupts = (typeof(ordered_interrupts)) reallocarray(ordered_interrupts, ordered_interrupts_count +1, sizeof(*ordered_interrupts));  // NOLINT(*-sizeof-expression)
                if (ordered_interrupts == NULL) {
                    perror("Failed to allocate memory for ordered interrupts");
                    kill(main_pid, SIGTERM);
                }

                ordered_interrupt_nos = (typeof(ordered_interrupt_nos)) reallocarray(ordered_interrupt_nos, ordered_interrupts_count +1, sizeof(*ordered_interrupt_nos));  // NOLINT(*-sizeof-expression)
                if (ordered_interrupt_nos == NULL) {
                    perror("Failed to allocate memory for ordered interrupt numbers");
                    kill(main_pid, SIGTERM);
                }

                SEOE(SCICreateInterrupt, sd, &ordered_interrupts[ordered_interrupts_count], ADAPTER_NO, &id, NO_CALLBACK, NO_ARG, NO_FLAGS);

                ordered_interrupt_nos[ordered_interrupts_count] = id;

                ordered_interrupts_count++;

                delivery_notification(STATUS_TYPE_SUCCESS, COMMAND_TYPE_CREATE, ORDER_TYPE_INTERRUPT, id);
                break;
            case ORDER_TYPE_DATA_INTERRUPT:
                ordered_data_interrupts = (typeof(ordered_data_interrupts)) reallocarray(ordered_data_interrupts, ordered_data_interrupts_count +1, sizeof(*ordered_data_interrupts));  // NOLINT(*-sizeof-expression)
                if (ordered_data_interrupts == NULL) {
                    perror("Failed to allocate memory for ordered data interrupts");
                    kill(main_pid, SIGTERM);
                }

                ordered_data_interrupt_nos = (typeof(ordered_data_interrupt_nos)) reallocarray(ordered_data_interrupt_nos, ordered_data_interrupts_count +1, sizeof(*ordered_data_interrupt_nos));  // NOLINT(*-sizeof-expression)
                if (ordered_data_interrupt_nos == NULL) {
                    perror("Failed to allocate memory for ordered data interrupt numbers");
                    kill(main_pid, SIGTERM);
                }

                SEOE(SCICreateDataInterrupt, sd, &ordered_data_interrupts[ordered_data_interrupts_count], ADAPTER_NO, &id, order_callback, NO_ARG, NO_FLAGS);

                ordered_data_interrupt_nos[ordered_data_interrupts_count] = id;

                ordered_data_interrupts_count++;

                delivery_notification(STATUS_TYPE_SUCCESS, COMMAND_TYPE_CREATE, ORDER_TYPE_DATA_INTERRUPT, id);
                break;
        }
    } else if (order->commandType == COMMAND_TYPE_DESTROY) {
        printf("Received destroy order\n");
        switch (order->orderType) {
            case ORDER_TYPE_SEGMENT:
            case ORDER_TYPE_GLOBAL_DMA_SEGMENT:
                new_ordered_segments = (typeof(new_ordered_segments)) malloc((ordered_segments_count - 1) * sizeof(*new_ordered_segments)); // NOLINT(*-sizeof-expression)
                if (new_ordered_segments == NULL) {
                    perror("Failed to allocate memory for new ordered segments");
                    delivery_notification(STATUS_TYPE_FAILURE, COMMAND_TYPE_DESTROY, ORDER_TYPE_SEGMENT, order->id);
                    kill(main_pid, SIGTERM);
                    return SCI_CALLBACK_CANCEL;
                }

                for (unsigned int i = 0, j = 0; i < ordered_segments_count; i++) {
                    if (SCIGetLocalSegmentId(ordered_segments[i]) != order->id) {
                        new_ordered_segments[j] = ordered_segments[i];
                        j++;
                    } else {
                        SEOE(SCIRemoveSegment, ordered_segments[i], NO_FLAGS);
                    }
                }

                tmp_ordered_segments = ordered_segments;
                ordered_segments = new_ordered_segments;
                free(tmp_ordered_segments);
                ordered_segments_count--;

                delivery_notification(STATUS_TYPE_SUCCESS, COMMAND_TYPE_DESTROY, order->orderType, order->id);
                break;
            case ORDER_TYPE_INTERRUPT:
                new_ordered_interrupts = (typeof(new_ordered_interrupts)) malloc((ordered_interrupts_count - 1) * sizeof(*new_ordered_interrupts)); // NOLINT(*-sizeof-expression)
                if (new_ordered_interrupts == NULL) {
                    perror("Failed to allocate memory for new ordered interrupts");
                    delivery_notification(STATUS_TYPE_FAILURE, COMMAND_TYPE_DESTROY, ORDER_TYPE_INTERRUPT, order->id);
                    kill(main_pid, SIGTERM);
                    return SCI_CALLBACK_CANCEL;
                }

                for (unsigned int i = 0, j = 0; i < ordered_interrupts_count; i++) {
                    if (ordered_interrupt_nos[i] != order->id) {
                        new_ordered_interrupts[j] = ordered_interrupts[i];
                        j++;
                    } else {
                        SEOE(SCIRemoveInterrupt, ordered_interrupts[i], NO_FLAGS);
                    }
                }

                tmp_ordered_interrupts = ordered_interrupts;
                ordered_interrupts = new_ordered_interrupts;
                free(tmp_ordered_interrupts);

                new_ordered_interrupt_nos = (typeof(new_ordered_interrupt_nos)) malloc((ordered_interrupts_count - 1) * sizeof(*new_ordered_interrupt_nos)); // NOLINT(*-sizeof-expression)
                if (new_ordered_interrupt_nos == NULL) {
                    perror("Failed to allocate memory for new ordered interrupt numbers");
                    delivery_notification(STATUS_TYPE_FAILURE, COMMAND_TYPE_DESTROY, ORDER_TYPE_INTERRUPT, order->id);
                    kill(main_pid, SIGTERM);
                    return SCI_CALLBACK_CANCEL;
                }

                for (unsigned int i = 0, j = 0; i < ordered_interrupts_count; i++) {
                    if (ordered_interrupt_nos[i] != order->id) {
                        new_ordered_interrupt_nos[j] = ordered_interrupt_nos[i];
                        j++;
                    }
                }

                tmp_ordered_interrupt_nos = ordered_interrupt_nos;
                ordered_interrupt_nos = new_ordered_interrupt_nos;
                free(tmp_ordered_interrupt_nos);

                delivery_notification(STATUS_TYPE_SUCCESS, COMMAND_TYPE_DESTROY, ORDER_TYPE_INTERRUPT, order->id);
                break;
            case ORDER_TYPE_DATA_INTERRUPT:
                new_ordered_data_interrupts = (typeof(new_ordered_data_interrupts)) malloc((ordered_data_interrupts_count - 1) * sizeof(*new_ordered_data_interrupts)); // NOLINT(*-sizeof-expression)
                if (new_ordered_data_interrupts == NULL) {
                    perror("Failed to allocate memory for new ordered data interrupts");
                    delivery_notification(STATUS_TYPE_FAILURE, COMMAND_TYPE_DESTROY, ORDER_TYPE_DATA_INTERRUPT, order->id);
                    kill(main_pid, SIGTERM);
                    return SCI_CALLBACK_CANCEL;
                }

                for (unsigned int i = 0, j = 0; i < ordered_data_interrupts_count; i++) {
                    if (ordered_data_interrupt_nos[i] != order->id) {
                        new_ordered_data_interrupts[j] = ordered_data_interrupts[i];
                        j++;
                    } else {
                        SEOE(SCIRemoveDataInterrupt, ordered_data_interrupts[i], NO_FLAGS);
                    }
                }

                tmp_ordered_data_interrupts = ordered_data_interrupts;
                ordered_data_interrupts = new_ordered_data_interrupts;
                free(tmp_ordered_data_interrupts);

                new_ordered_data_interrupt_nos = (typeof(new_ordered_data_interrupt_nos)) malloc((ordered_data_interrupts_count - 1) * sizeof(*new_ordered_data_interrupt_nos)); // NOLINT(*-sizeof-expression)
                if (new_ordered_data_interrupt_nos == NULL) {
                    perror("Failed to allocate memory for new ordered data interrupt numbers");
                    delivery_notification(STATUS_TYPE_FAILURE, COMMAND_TYPE_DESTROY, ORDER_TYPE_DATA_INTERRUPT, order->id);
                    kill(main_pid, SIGTERM);
                    return SCI_CALLBACK_CANCEL;
                }

                for (unsigned int i = 0, j = 0; i < ordered_data_interrupts_count; i++) {
                    if (ordered_data_interrupt_nos[i] != order->id) {
                        new_ordered_data_interrupt_nos[j] = ordered_data_interrupt_nos[i];
                        j++;
                    }
                }

                tmp_ordered_data_interrupt_nos = ordered_data_interrupt_nos;
                ordered_data_interrupt_nos = new_ordered_data_interrupt_nos;
                free(tmp_ordered_data_interrupt_nos);

                delivery_notification(STATUS_TYPE_SUCCESS, COMMAND_TYPE_DESTROY, ORDER_TYPE_DATA_INTERRUPT, order->id);
                break;
        }
    } else {
        fprintf(stderr, "Received invalid command type %d from delivery\n", order->commandType);
        kill(main_pid, SIGTERM);
    }

    return SCI_CALLBACK_CONTINUE;
}

int main(int argc, char *argv[]) {
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

    for (unsigned int i = 0; i < ordered_segments_count; i++) {
        SEOE(SCIRemoveSegment, ordered_segments[i], NO_FLAGS);
    }

    for (unsigned int i = 0; i < ordered_interrupts_count; i++) {
        SEOE(SCIRemoveInterrupt, ordered_interrupts[i], NO_FLAGS);
    }

    for (unsigned int i = 0; i < ordered_data_interrupts_count; i++) {
        SEOE(SCIRemoveDataInterrupt, ordered_data_interrupts[i], NO_FLAGS);
    }

    SEOE(SCIDisconnectDataInterrupt, delivery_interrupt, NO_FLAGS);

    SEOE(SCIRemoveDataInterrupt, order_interrupt, NO_FLAGS);

    SEOE(SCIClose, sd, NO_FLAGS);
    SCITerminate();
}