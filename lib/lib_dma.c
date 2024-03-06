#include <stdlib.h>
#include <stdio.h>
#include "lib_dma.h"
#include "sisci_api.h"
#include "sisci_glob_defs.h"
#include "error_util.h"


void send_dma_segment(sci_dma_queue_t *dma_queue, sci_local_segment_t local_segment, sci_remote_segment_t remote_segment, size_t size, bool use_sysdma) {
    DEBUG_PRINT("Sending DMA segment\n");
    sci_dma_queue_state_t dma_queue_state;
    sci_error_t error;
    unsigned int flags = use_sysdma ? SCI_FLAG_DMA_SYSDMA : NO_FLAGS;

    SCIStartDmaTransfer(
            *dma_queue,
            local_segment,
            remote_segment,
            NO_OFFSET,
            size,
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

void dma_init(sci_desc_t v_dev, sci_remote_segment_t remote_segment, size_t size, sci_dma_queue_t *dma_queue, sci_map_t remote_map) {
    DEBUG_PRINT("Initializing DMA\n");
    sci_error_t error;
    sci_dma_queue_state_t dma_q_state;

    SCIMapRemoteSegment(remote_segment, &remote_map, NO_OFFSET, size, NO_SUG_ADDR, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIMapRemoteSegment", true);

    SCICreateDMAQueue(v_dev, dma_queue, ADAPTER_NO, 1, NO_FLAGS, &error);
    print_sisci_error(&error, "SCICreateDMAQueue", true);

    dma_q_state = SCIDMAQueueState(*dma_queue);
    if (dma_q_state == SCI_DMAQUEUE_IDLE){
        DEBUG_PRINT("DMA queue is idle\n");
    }
    else {
        fprintf(stderr, "DMA queue is not idle\n");
        exit(EXIT_FAILURE);
    }
}

void dma_destroy(sci_map_t local_map, sci_map_t remote_map, sci_dma_queue_t dma_queue) {
    DEBUG_PRINT("Destroying DMA queue\n");
    sci_error_t error;
    SCIRemoveDMAQueue(dma_queue, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIRemoveDMAQueue", false);

    SCIUnmapSegment(local_map, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIUnmapSegment", false);

    SCIUnmapSegment(remote_map, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIUnmapSegment", false);
}

void dma_channel_destroy(sci_dma_channel_t dma_channel, sci_local_segment_t local_segment, sci_remote_segment_t remote_segment) {
    DEBUG_PRINT("Destroying DMA channel\n");
    sci_error_t error;

    SCIUnprepareRemoteSegmentForDMA(dma_channel, remote_segment, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIUnprepareRemoteSegmentForDMA", true);

    SCIUnprepareLocalSegmentForDMA(dma_channel, local_segment, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIUnprepareLocalSegmentForDMA", true);

    SCIReturnDMAChannel(dma_channel, &error);
    print_sisci_error(&error, "SCIReturnDMAChannel", true);
}

void dma_channel_init(sci_desc_t v_dev, sci_local_segment_t local_segment, sci_remote_segment_t remote_segment, unsigned int channel_id, sci_dma_queue_t *dma_queue, sci_dma_channel_t dma_channel) {
    DEBUG_PRINT("Requesting DMA channel\n");
    sci_error_t error;

    SCIRequestDMAChannel(v_dev, &dma_channel, ADAPTER_NO, SCI_DMA_TYPE_SYSTEM, channel_id, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIRequestDMAChannel", true);

    SCIAssignDMAChannel(dma_channel, *dma_queue, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIAssignDMAChannel", true);

    SCIPrepareLocalSegmentForDMA(dma_channel, local_segment, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIPrepareLocalSegmentForDMA", true);

    SCIPrepareRemoteSegmentForDMA(dma_channel, remote_segment, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIPrepareRemoteSegmentForDMA", true);
}