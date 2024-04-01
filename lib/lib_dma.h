#ifndef KRITJOLIB_SISCI_DMA
#define KRITJOLIB_SISCI_DMA

#include <stdbool.h>

#include "sisci_types.h"
#include "sisci_arg_types.h"

void transfer_dma_segment(sci_dma_queue_t dma_queue, segment_local_args_t *local, segment_remote_args_t *remote, sci_cb_dma_t callback, void *callback_arg, bool wait, bool use_local_addr, unsigned int flags);
void wait_for_dma_queue(sci_dma_queue_t dma_queue, unsigned int flags);
void init_dma(sci_desc_t v_dev, sci_dma_queue_t *dma_queue, segment_remote_args_t *remote, unsigned int flags);
void destroy_dma(sci_dma_queue_t dma_queue, sci_map_t remote_map, unsigned int flags);

#endif //KRITJOLIB_SISCI_DMA
