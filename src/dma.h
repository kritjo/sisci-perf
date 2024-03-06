#ifndef KRITJO_SISCI_DMA
#define KRITJO_SISCI_DMA

#include <stdbool.h>

#include "sisci_types.h"
#include "rdma_buff.h"

void send_dma_buff(sci_dma_queue_t *dma_queue, sci_local_segment_t local_segment, sci_remote_segment_t remote_segment, bool use_sysdma);
void dma(sci_desc_t v_dev, sci_remote_segment_t remote_segment, bool use_sysdma, unsigned int channel_id);

#endif //KRITJO_SISCI_DMA
