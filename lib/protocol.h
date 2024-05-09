#ifndef SISCI_PERF_PROTOCOL_H
#define SISCI_PERF_PROTOCOL_H

#include <stddef.h>

#define DELIVERY_INTERRUPT_NO 9573

typedef enum {
    ORDER_TYPE_SEGMENT,
    ORDER_TYPE_GLOBAL_DMA_SEGMENT,
    ORDER_TYPE_INTERRUPT,
    ORDER_TYPE_DATA_INTERRUPT,
} order_type_t;

typedef enum {
    COMMAND_TYPE_CREATE,
    COMMAND_TYPE_DESTROY,
} command_type_t;

typedef struct {
    order_type_t orderType;
    command_type_t commandType;

    size_t size;

    unsigned int id;
} order_t;

typedef enum {
    STATUS_TYPE_PROTOCOL, // Meta status type
    STATUS_TYPE_SUCCESS,
    STATUS_TYPE_FAILURE,
} delivery_status_t;

typedef struct {
    delivery_status_t status;

    order_type_t deliveryType;
    command_type_t commandType;

    unsigned int id;
    unsigned int nodeId;

    char message[40];
} delivery_t;

#endif //SISCI_PERF_PROTOCOL_H
