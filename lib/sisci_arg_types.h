#ifndef SISCI_PLAYGROUND_SISCI_ARG_TYPES_H
#define SISCI_PLAYGROUND_SISCI_ARG_TYPES_H

#include <stddef.h>

#include "sisci_types.h"

typedef struct {
    sci_local_segment_t segment;
    size_t segment_size;
    sci_map_t map;
    void *address;
} segment_local_args_t;

typedef struct {
    sci_remote_segment_t segment;
    size_t segment_size;
    sci_map_t map;
    void *address;
} segment_remote_args_t;

typedef struct {
    sci_cb_local_segment_t callback;
    void *arg;
} segment_callback_args_t;

typedef struct {
    segment_local_args_t local;
    segment_remote_args_t remote;
    sci_dma_type_t dma_mode;
    segment_callback_args_t callback_args;
} segment_args_t;

typedef struct {
    sci_desc_t v_dev;
    sci_remote_segment_t remote_segment;
    sci_local_segment_t local_segment;
    size_t size;
    sci_dma_queue_t dma_queue;
    sci_map_t remote_map;
    sci_map_t local_map;
    bool use_sysdma;
} dma_args_t;

#endif //SISCI_PLAYGROUND_SISCI_ARG_TYPES_H
