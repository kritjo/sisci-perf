#ifndef SISCI_PLAYGROUND_SISCI_ARG_TYPES_H
#define SISCI_PLAYGROUND_SISCI_ARG_TYPES_H

#include <stddef.h>

#include "sisci_types.h"

#define OUT_NON_NULL_WARNING(var) do { \
    if (var != 0) { fprintf(stderr, "Warning: OUT PARAMETER %s is not NULL\n", #var); } \
} while(0)

typedef struct {
    sci_local_segment_t segment;
    size_t segment_size;
    sci_map_t map;
    void *address;
    unsigned int offset;
} segment_local_args_t;

typedef struct {
    sci_remote_segment_t segment;
    size_t segment_size;
    sci_map_t map;
    volatile void *address;
    unsigned int offset;
} segment_remote_args_t;

typedef struct {
    sci_cb_local_segment_t function;
    void *arg;
} segment_callback_args_t;

#endif //SISCI_PLAYGROUND_SISCI_ARG_TYPES_H
