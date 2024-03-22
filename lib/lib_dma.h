#ifndef KRITJOLIB_SISCI_DMA
#define KRITJOLIB_SISCI_DMA

#include <stdbool.h>

#include "sisci_types.h"
#include "sisci_arg_types.h"

void send_dma_segment(sci_dma_queue_t dma_queue, segment_local_args_t *local, segment_remote_args_t *remote, sci_cb_dma_t callback, void *callback_arg, unsigned int flags);
void dma_init(sci_desc_t v_dev, sci_dma_queue_t *dma_queue, segment_remote_args_t *remote, unsigned int flags);
void dma_destroy(sci_dma_queue_t dma_queue, sci_map_t remote_map);

#endif //KRITJOLIB_SISCI_DMA
