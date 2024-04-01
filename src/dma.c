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

#define SEND_SEG_ID 4589

void dma_send_test(sci_desc_t v_dev, sci_remote_segment_t remote_segment, bool use_sysdma, bool use_globdma) {
    DEBUG_PRINT("Sending DMA segment using ");
    if (use_sysdma) DEBUG_PRINT("System DMA\n");
    else if (use_globdma) DEBUG_PRINT("Global DMA\n");
    else DEBUG_PRINT("Any DMA\n");
    unsigned int flags = use_sysdma ? SCI_FLAG_DMA_SYSDMA : use_globdma ? SCI_FLAG_DMA_GLOBAL : NO_FLAGS;

    sci_error_t error;
    rdma_buff_t* local_map_address;
    sci_dma_queue_t dma_queue;

    segment_remote_args_t remote = {0};
    remote.segment = remote_segment;
    remote.segment_size = sizeof(rdma_buff_t);
    remote.address = NO_SUG_ADDR;
    remote.offset = NO_OFFSET;
    remote.map = 0;

    segment_local_args_t local = {0};
    local.segment_size = sizeof(rdma_buff_t);

    init_dma(v_dev, &dma_queue, &remote, flags);

    SCICreateSegment(v_dev, &local.segment, SEND_SEG_ID, local.segment_size, NO_CALLBACK, NO_ARG, SCI_FLAG_PRIVATE, &error);
    print_sisci_error(&error, "SCICreateSegment", true);

    SCIPrepareSegment(local.segment, ADAPTER_NO, NO_FLAGS, &error);

    local_map_address = (rdma_buff_t*)SCIMapLocalSegment(local.segment, &local.map, NO_OFFSET, local.segment_size, NO_SUG_ADDR, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIMapLocalSegment", true);

    local_map_address->done = 0;
    strcpy(local_map_address->word, "OK");

    send_dma_segment(dma_queue, &local, &remote, NO_CALLBACK, NO_ARG, false, flags);

    local_map_address->done = 1;
    send_dma_segment(dma_queue, &local, &remote, NO_CALLBACK, NO_ARG, false, flags);

    SCIUnmapSegment(local.map, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIUnmapSegment", false);

    SCIRemoveSegment(local.segment, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIRemoveSegment", false);

    destroy_dma(dma_queue, remote.map, flags);
}