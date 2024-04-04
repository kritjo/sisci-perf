#ifndef KRITJOLIB_SISCI_DMA
#define KRITJOLIB_SISCI_DMA

#include <stdbool.h>

#include "sisci_types.h"
#include "sisci_arg_types.h"

void transfer_dma_segment(sci_dma_queue_t dma_queue, segment_local_args_t *local, segment_remote_args_t *remote, sci_cb_dma_t callback, void *callback_arg, bool wait, bool use_local_addr, unsigned int start_transfer_flags);
void wait_for_dma_queue(sci_dma_queue_t dma_queue);
void init_dma(sci_desc_t v_dev, sci_dma_queue_t *dma_queue, segment_remote_args_t *remote, bool use_global_dma);
void destroy_dma(sci_dma_queue_t dma_queue, sci_map_t remote_map, bool use_global_dma);
void init_dma_channel(sci_desc_t v_dev, sci_dma_channel_t *dma_channel, sci_dma_type_t dma_type, sci_dma_queue_t dma_queue);
void destroy_dma_channel(sci_dma_channel_t dma_channel);

#endif //KRITJOLIB_SISCI_DMA
