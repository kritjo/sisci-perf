#include <stdlib.h>
#include <stdio.h>
#include "lib_dma.h"
#include "sisci_api.h"
#include "sisci_glob_defs.h"
#include "error_util.h"

void wait_for_dma_queue(sci_dma_queue_t dma_queue) {
    sci_error_t error;
    SCIWaitForDMAQueue(dma_queue, SCI_INFINITE_TIMEOUT, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIWaitForDMAQueue", true);

    sci_dma_queue_state_t dma_queue_state = SCIDMAQueueState(dma_queue);
    if (dma_queue_state == SCI_DMAQUEUE_DONE)
        printf("Transfer successful!\n");
    else {
        fprintf(stderr, "Transfer failed!\n");
        exit(EXIT_FAILURE);
    }
}

void transfer_dma_segment(
        sci_dma_queue_t dma_queue,
        segment_local_args_t *local,
        segment_remote_args_t *remote,
        sci_cb_dma_t callback,
        void *callback_arg,
        bool wait,
        bool use_local_addr,
        unsigned int start_transfer_flags) {
    sci_error_t error;

    if (local->segment_size != remote->segment_size) {
        fprintf(stderr, "Warning: local segment size (%ld) does not match remote segment size (%ld)\n", local->segment_size, remote->segment_size);
        exit(EXIT_FAILURE);
    }

    DEBUG_PRINT("Sending DMA segment\n");

    if (use_local_addr) {
        SCIStartDmaTransferMem(
                dma_queue,
                local->address,
                remote->segment,
                local->segment_size,
                remote->offset,
                callback,
                callback_arg,
                start_transfer_flags,
                &error);
        print_sisci_error(&error, "SCIStartDmaTransferMem", true);
    } else {
        SCIStartDmaTransfer(
                dma_queue,
                local->segment,
                remote->segment,
                local->offset,
                local->segment_size,
                remote->offset,
                callback,
                callback_arg,
                start_transfer_flags,
                &error);
        print_sisci_error(&error, "SCIStartDmaTransfer", true);
    }

    if (wait) {
        wait_for_dma_queue(dma_queue);
    } else {
        printf("Transfer started, not waiting for completion\n");
    }
}

void init_dma(sci_desc_t v_dev, sci_dma_queue_t *dma_queue, segment_remote_args_t *remote, bool use_global_dma) {
    OUT_NON_NULL_WARNING(remote->map);
    DEBUG_PRINT("Initializing DMA\n");
    sci_error_t error;
    sci_dma_queue_state_t dma_q_state;

    if (!use_global_dma) {
        SCIMapRemoteSegment(remote->segment, &remote->map, remote->offset, remote->segment_size, &remote->address,
                            NO_FLAGS, &error);
        print_sisci_error(&error, "SCIMapRemoteSegment", true);
    }

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

void destroy_dma(sci_dma_queue_t dma_queue, sci_map_t remote_map, bool use_global_dma) {
    DEBUG_PRINT("Destroying DMA queue\n");
    sci_error_t error;
    SCIRemoveDMAQueue(dma_queue, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIRemoveDMAQueue", false);

    if (!use_global_dma) {
        SCIUnmapSegment(remote_map, NO_FLAGS, &error);
        print_sisci_error(&error, "SCIUnmapSegment", false);
    }
}


void init_dma_channel(sci_desc_t v_dev, sci_dma_channel_t *dma_channel, sci_dma_type_t dma_type, sci_dma_queue_t dma_queue) {
    sci_error_t error;

    SCIRequestDMAChannel(v_dev, dma_channel, ADAPTER_NO, dma_type, SCI_DMA_CHANNEL_ID_DONTCARE, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIRequestDMAChannel", true);

    SCIAssignDMAChannel(*dma_channel, dma_queue, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIAssignDMAChannel", true);
}

void destroy_dma_channel(sci_dma_channel_t dma_channel) {
    sci_error_t error;

    SCIReturnDMAChannel(dma_channel, &error);
    print_sisci_error(&error, "SCIReleaseDMAChannel", false);
}