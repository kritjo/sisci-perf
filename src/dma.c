#include <stdbool.h>
#include <string.h>

#include "sisci_types.h"
#include "sisci_error.h"
#include "sisci_api.h"

#include "lib_dma.h"
#include "dma.h"
#include "rdma_buff.h"
#include "error_util.h"
#include "sisci_glob_defs.h"

#define SEND_SEG_ID 4589

void dma_send_test(sci_desc_t v_dev, sci_remote_segment_t remote_segment, bool use_sysdma, unsigned int channel_id) {
    sci_local_segment_t local_segment;
    rdma_buff_t* local_map_address;
    sci_map_t local_map;
    sci_map_t remote_map = NULL;
    sci_error_t error;
    sci_dma_queue_t dma_queue;
    sci_dma_channel_t dma_channel = NULL;

    dma_init(v_dev, remote_segment, sizeof(rdma_buff_t), &dma_queue, remote_map);

    SCICreateSegment(v_dev, &local_segment, SEND_SEG_ID, sizeof(rdma_buff_t), NO_CALLBACK, NO_ARG, SCI_FLAG_PRIVATE, &error);
    print_sisci_error(&error, "SCICreateSegment", true);

    SCIPrepareSegment(local_segment, ADAPTER_NO, NO_FLAGS, &error);

    local_map_address = (rdma_buff_t*)SCIMapLocalSegment(local_segment, &local_map, NO_OFFSET, sizeof(rdma_buff_t), NO_SUG_ADDR, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIMapLocalSegment", true);

    local_map_address->done = 0;
    strcpy(local_map_address->word, "OK");

    send_dma_buff(&dma_queue, local_segment, remote_segment, sizeof(rdma_buff_t), use_sysdma);

    local_map_address->done = 1;
    send_dma_buff(&dma_queue, local_segment, remote_segment, sizeof(rdma_buff_t), use_sysdma);

    if (channel_id != 1) dma_channel_init(v_dev, local_segment, remote_segment, channel_id, &dma_queue, dma_channel);

    send_dma_buff(&dma_queue, local_segment, remote_segment, sizeof(rdma_buff_t), use_sysdma);

    local_map_address->done = 1;
    send_dma_buff(&dma_queue, local_segment, remote_segment, sizeof(rdma_buff_t), use_sysdma);

    SCIRemoveSegment(local_segment, NO_FLAGS, &error);

    if (channel_id != 1) dma_channel_destroy(dma_channel, local_segment, remote_segment);
    dma_destroy(local_map, remote_map, dma_queue);
}