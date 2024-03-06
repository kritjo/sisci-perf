#include <stdlib.h>
#include <stdio.h>
#include "lib_dma.h"
#include "sisci_api.h"
#include "sisci_glob_defs.h"
#include "error_util.h"

void send_dma_segment(dma_args_t *args) {
    DEBUG_PRINT("Sending DMA segment\n");
    sci_dma_queue_state_t dma_queue_state;
    sci_error_t error;
    unsigned int flags = args->use_sysdma ? SCI_FLAG_DMA_SYSDMA : NO_FLAGS;

    SCIStartDmaTransfer(
            args->dma_queue,
            args->local_segment,
            args->remote_segment,
            NO_OFFSET,
            args->size,
            NO_OFFSET,
            NO_CALLBACK,
            NO_ARG,
            flags,
            &error);
    print_sisci_error(&error, "SCIStartDmaTransferMem", true);

    SCIWaitForDMAQueue(args->dma_queue, SCI_INFINITE_TIMEOUT, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIWaitForDMAQueue", true);

    dma_queue_state = SCIDMAQueueState(args->dma_queue);
    if (dma_queue_state == SCI_DMAQUEUE_DONE)
        printf("Transfer successful!\n");
    else {
        fprintf(stderr, "Transfer failed!\n");
        exit(EXIT_FAILURE);
    }
}

void dma_init(dma_args_t *args) {
    DEBUG_PRINT("Initializing DMA\n");
    sci_error_t error;
    sci_dma_queue_state_t dma_q_state;

    SCIMapRemoteSegment(args->remote_segment, &args->remote_map, NO_OFFSET, args->size, NO_SUG_ADDR, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIMapRemoteSegment", true);

    SCICreateDMAQueue(args->v_dev, &args->dma_queue, ADAPTER_NO, 1, NO_FLAGS, &error);
    print_sisci_error(&error, "SCICreateDMAQueue", true);

    dma_q_state = SCIDMAQueueState(args->dma_queue);
    if (dma_q_state == SCI_DMAQUEUE_IDLE){
        DEBUG_PRINT("DMA queue is idle\n");
    }
    else {
        fprintf(stderr, "DMA queue is not idle\n");
        exit(EXIT_FAILURE);
    }
}

void dma_destroy(dma_args_t *args) {
    DEBUG_PRINT("Destroying DMA queue\n");
    sci_error_t error;
    SCIRemoveDMAQueue(args->dma_queue, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIRemoveDMAQueue", false);

    SCIUnmapSegment(args->local_map, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIUnmapSegment", false);

    SCIUnmapSegment(args->remote_map, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIUnmapSegment", false);
}

void dma_channel_destroy(dma_args_t *args, dma_channel_args_t *ch_args) {
    DEBUG_PRINT("Destroying DMA channel\n");
    sci_error_t error;

    SCIUnprepareRemoteSegmentForDMA(ch_args->dma_channel, args->remote_segment, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIUnprepareRemoteSegmentForDMA", true);

    SCIUnprepareLocalSegmentForDMA(ch_args->dma_channel, args->local_segment, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIUnprepareLocalSegmentForDMA", true);

    SCIReturnDMAChannel(ch_args->dma_channel, &error);
    print_sisci_error(&error, "SCIReturnDMAChannel", true);
}

void dma_channel_init(dma_args_t *args, dma_channel_args_t *ch_args) {
    DEBUG_PRINT("Requesting DMA channel\n");
    sci_error_t error;
    sci_dma_type_t dma_type = args->use_sysdma ? SCI_DMA_TYPE_SYSTEM : SCI_DMA_TYPE_ADAPTER;

    SCIRequestDMAChannel(args->v_dev, &ch_args->dma_channel, ADAPTER_NO, dma_type, ch_args->channel_id, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIRequestDMAChannel", true);

    SCIAssignDMAChannel(ch_args->dma_channel, args->dma_queue, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIAssignDMAChannel", true);

    SCIPrepareLocalSegmentForDMA(ch_args->dma_channel, args->local_segment, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIPrepareLocalSegmentForDMA", true);

    SCIPrepareRemoteSegmentForDMA(ch_args->dma_channel, args->remote_segment, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIPrepareRemoteSegmentForDMA", true);
}