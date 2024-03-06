#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sisci_types.h"
#include "sisci_error.h"
#include "sisci_api.h"

#include "dma.h"
#include "rdma_buff.h"
#include "error_util.h"
#include "sisci_glob_defs.h"

#define SEND_SEG_ID 4589


void send_dma_buff(sci_dma_queue_t *dma_queue, sci_local_segment_t local_segment, sci_remote_segment_t remote_segment, bool use_sysdma) {
    sci_dma_queue_state_t dma_queue_state;
    sci_error_t error;
    unsigned int flags = use_sysdma ? SCI_FLAG_DMA_SYSDMA : NO_FLAGS;

    SCIStartDmaTransfer(
            *dma_queue,
            local_segment,
            remote_segment,
            NO_OFFSET,
            sizeof(rdma_buff_t),
            NO_OFFSET,
            NO_CALLBACK,
            NO_ARG,
            flags,
            &error);
    print_sisci_error(&error, "SCIStartDmaTransferMem", true);

    SCIWaitForDMAQueue(*dma_queue, SCI_INFINITE_TIMEOUT, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIWaitForDMAQueue", true);

    dma_queue_state = SCIDMAQueueState(*dma_queue);
    if (dma_queue_state == SCI_DMAQUEUE_DONE)
        printf("Transfer successful!\n");
    else {
        fprintf(stderr, "Transfer failed!\n");
        exit(EXIT_FAILURE);
    }
}

void dma(sci_desc_t v_dev, sci_remote_segment_t remote_segment, bool use_sysdma, unsigned int channel_id) {
    sci_dma_queue_t dma_queue;
    sci_error_t error;
    sci_dma_queue_state_t dma_q_state;
    sci_dma_channel_t dma_channel;
    sci_local_segment_t local_segment;
    rdma_buff_t* local_map_address;
    sci_map_t local_map;
    sci_map_t remote_map;

    SCICreateSegment(v_dev, &local_segment, SEND_SEG_ID, sizeof(rdma_buff_t), NO_CALLBACK, NO_ARG, SCI_FLAG_PRIVATE, &error);
    print_sisci_error(&error, "SCICreateSegment", true);

    SCIPrepareSegment(local_segment, ADAPTER_NO, NO_FLAGS, &error);

    local_map_address = (rdma_buff_t*)SCIMapLocalSegment(local_segment, &local_map, NO_OFFSET, sizeof(rdma_buff_t), NO_SUG_ADDR, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIMapLocalSegment", true);

    local_map_address->done = 0;
    strcpy(local_map_address->word, "OK");

    SCIMapRemoteSegment(remote_segment, &remote_map, NO_OFFSET, sizeof(rdma_buff_t), NO_SUG_ADDR, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIMapRemoteSegment", true);

    SCICreateDMAQueue(v_dev, &dma_queue, ADAPTER_NO, 1, NO_FLAGS, &error);
    print_sisci_error(&error, "SCICreateDMAQueue", true);

    if (channel_id != -1) {
        SCIRequestDMAChannel(v_dev, &dma_channel, ADAPTER_NO, SCI_DMA_TYPE_SYSTEM, channel_id, NO_FLAGS, &error);
        print_sisci_error(&error, "SCIRequestDMAChannel", true);

        SCIAssignDMAChannel(dma_channel, dma_queue, NO_FLAGS, &error);
        print_sisci_error(&error, "SCIAssignDMAChannel", true);

        SCIPrepareLocalSegmentForDMA(dma_channel, local_segment, NO_FLAGS, &error);
        print_sisci_error(&error, "SCIPrepareLocalSegmentForDMA", true);

        SCIPrepareRemoteSegmentForDMA(dma_channel, remote_segment, NO_FLAGS, &error);
        print_sisci_error(&error, "SCIPrepareRemoteSegmentForDMA", true);
    }

    dma_q_state = SCIDMAQueueState(dma_queue);
    if (dma_q_state == SCI_DMAQUEUE_IDLE)
        printf("DMA queue is idle\n");
    else {
        fprintf(stderr, "DMA queue is not idle\n");
        exit(EXIT_FAILURE);
    }

    send_dma_buff(&dma_queue, local_segment, remote_segment, use_sysdma);

    local_map_address->done = 1;
    send_dma_buff(&dma_queue, local_segment, remote_segment, use_sysdma);

    if (channel_id != -1) {
        SCIUnprepareRemoteSegmentForDMA(dma_channel, remote_segment, NO_FLAGS, &error);
        print_sisci_error(&error, "SCIUnprepareRemoteSegmentForDMA", true);

        SCIUnprepareLocalSegmentForDMA(dma_channel, local_segment, NO_FLAGS, &error);
        print_sisci_error(&error, "SCIUnprepareLocalSegmentForDMA", true);

        SCIReturnDMAChannel(dma_channel, &error);
        print_sisci_error(&error, "SCIReturnDMAChannel", true);
    }

    SCIRemoveDMAQueue(dma_queue, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIRemoveDMAQueue", false);

    SCIUnmapSegment(local_map, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIUnmapSegment", false);

    SCIUnmapSegment(remote_map, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIUnmapSegment", false);
}