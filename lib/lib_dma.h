#ifndef KRITJOLIB_SISCI_DMA
#define KRITJOLIB_SISCI_DMA

#include <stdbool.h>

#include "sisci_types.h"

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

typedef struct {
    unsigned int channel_id;
    sci_dma_channel_t dma_channel;
} dma_channel_args_t;

void send_dma_segment(dma_args_t *args);
void dma_init(dma_args_t *args);
void dma_destroy(dma_args_t *args);
void dma_channel_destroy(dma_args_t *args, dma_channel_args_t *ch_args);
void dma_channel_init(dma_args_t *args, dma_channel_args_t *ch_args);

#endif //KRITJOLIB_SISCI_DMA
