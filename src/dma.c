#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "sisci_types.h"
#include "sisci_error.h"
#include "sisci_api.h"

#include "lib_dma.h"
#include "dma.h"
#include "rdma_buff.h"
#include "error_util.h"
#include "sisci_glob_defs.h"
#include "args_parser.h"

#define SEND_SEG_ID 4589

void dma_send_test(sci_desc_t v_dev, sci_remote_segment_t remote_segment, bool use_sysdma, unsigned int channel_id) {
    DEBUG_PRINT("Sending DMA segment using %s\n", use_sysdma ? "sysdma" : "dma");
    if (channel_id != UNINITIALIZED_ARG) DEBUG_PRINT("Channel id: %d\n", channel_id);
    sci_error_t error;
    rdma_buff_t* local_map_address;

    sci_local_segment_t local_segment = NULL;
    sci_map_t local_map = NULL;
    sci_map_t remote_map = NULL;
    sci_dma_queue_t dma_queue = NULL;
    sci_dma_channel_t dma_channel = NULL;
    dma_args_t dma_args = {v_dev, remote_segment, local_segment, sizeof(rdma_buff_t), dma_queue, remote_map, local_map, use_sysdma};
    dma_channel_args_t dma_channel_args = {channel_id, dma_channel};

    dma_init(&dma_args);

    SCICreateSegment(v_dev, &local_segment, SEND_SEG_ID, sizeof(rdma_buff_t), NO_CALLBACK, NO_ARG, SCI_FLAG_PRIVATE, &error);
    print_sisci_error(&error, "SCICreateSegment", true);

    SCIPrepareSegment(local_segment, ADAPTER_NO, NO_FLAGS, &error);

    local_map_address = (rdma_buff_t*)SCIMapLocalSegment(local_segment, &local_map, NO_OFFSET, sizeof(rdma_buff_t), NO_SUG_ADDR, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIMapLocalSegment", true);

    local_map_address->done = 0;
    strcpy(local_map_address->word, "OK");

    if (channel_id != UNINITIALIZED_ARG) dma_channel_init(&dma_args, &dma_channel_args);

    send_dma_segment(&dma_args);

    local_map_address->done = 1;
    send_dma_segment(&dma_args);

    SCIRemoveSegment(local_segment, NO_FLAGS, &error);

    if (channel_id != UNINITIALIZED_ARG) dma_channel_destroy(&dma_args, &dma_channel_args);
    dma_destroy(&dma_args);
}