#ifndef KRITJO_SISCI_DMA
#define KRITJO_SISCI_DMA

#include <stdbool.h>

#include "sisci_types.h"
#include "rdma_buff.h"

void dma_send_test(sci_desc_t v_dev, sci_remote_segment_t remote_segment, bool use_sysdma, unsigned int channel_id);
void send_dma_buff(sci_dma_queue_t *dma_queue, sci_local_segment_t local_segment, sci_remote_segment_t remote_segment, bool use_sysdma);
void dma_init(sci_desc_t v_dev, sci_remote_segment_t remote_segment, sci_dma_queue_t *dma_queue, sci_map_t remote_map);
void dma_destroy(sci_map_t local_map, sci_map_t remote_map, sci_dma_queue_t dma_queue);
void dma_channel_destroy(sci_dma_channel_t dma_channel, sci_local_segment_t local_segment, sci_remote_segment_t remote_segment);
void dma_channel_init(sci_desc_t v_dev, sci_local_segment_t local_segment, sci_remote_segment_t remote_segment, unsigned int channel_id, sci_dma_queue_t *dma_queue, sci_dma_channel_t dma_channel);

#endif //KRITJO_SISCI_DMA
