#include <stdlib.h>
#include <stdio.h>
#include "lib_dma.h"
#include "sisci_api.h"
#include "sisci_glob_defs.h"
#include "error_util.h"

void send_dma_segment(
        sci_dma_queue_t dma_queue,
        segment_local_args_t *local,
        segment_remote_args_t *remote,
        sci_cb_dma_t callback,
        void *callback_arg,
        unsigned int flags) {
    sci_error_t error;

    UNUSED_NON_NULL_WARNING(local->address);
    UNUSED_NON_NULL_WARNING(local->map);
    UNUSED_NON_NULL_WARNING(remote->address);
    UNUSED_NON_NULL_WARNING(remote->map);

    if (local->segment_size != remote->segment_size) {
        fprintf(stderr, "Warning: local segment size (%ld) does not match remote segment size (%ld)\n", local->segment_size, remote->segment_size);
        exit(EXIT_FAILURE);
    }

    DEBUG_PRINT("Sending DMA segment\n");
    SCIStartDmaTransfer(
            dma_queue,
            local->segment,
            remote->segment,
            local->offset,
            local->segment_size,
            remote->offset,
            callback,
            callback_arg,
            flags,
            &error);
    print_sisci_error(&error, "SCIStartDmaTransferMem", true);

    SCIWaitForDMAQueue(dma_queue, SCI_INFINITE_TIMEOUT, flags, &error);
    print_sisci_error(&error, "SCIWaitForDMAQueue", true);

    sci_dma_queue_state_t dma_queue_state = SCIDMAQueueState(dma_queue);
    if (dma_queue_state == SCI_DMAQUEUE_DONE)
        printf("Transfer successful!\n");
    else {
        fprintf(stderr, "Transfer failed!\n");
        exit(EXIT_FAILURE);
    }
}

void init_dma(sci_desc_t v_dev, sci_dma_queue_t *dma_queue, segment_remote_args_t *remote, unsigned int flags) {
    OUT_NON_NULL_WARNING(remote->map);
    DEBUG_PRINT("Initializing DMA\n");
    sci_error_t error;
    sci_dma_queue_state_t dma_q_state;

    if (!(flags & SCI_FLAG_DMA_GLOBAL)) {
        SCIMapRemoteSegment(remote->segment, &remote->map, remote->offset, remote->segment_size, &remote->address,
                            flags, &error);
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

void destroy_dma(sci_dma_queue_t dma_queue, sci_map_t remote_map, unsigned int flags) {
    DEBUG_PRINT("Destroying DMA queue\n");
    sci_error_t error;
    SCIRemoveDMAQueue(dma_queue, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIRemoveDMAQueue", false);

    if (!(flags & SCI_FLAG_DMA_GLOBAL)) {
        SCIUnmapSegment(remote_map, NO_FLAGS, &error);
        print_sisci_error(&error, "SCIUnmapSegment", false);
    }
}
