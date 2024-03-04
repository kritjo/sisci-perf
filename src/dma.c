#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "sisci_types.h"
#include "sisci_error.h"
#include "sisci_api.h"

#include "dma.h"
#include "rdma_buff.h"
#include "error_util.h"
#include "sisci_glob_defs.h"


void send_dma_buff(sci_dma_queue_t *dma_queue, rdma_buff_t *rdma_buff, sci_remote_segment_t *remote_segment, bool use_sysdma) {
    sci_dma_queue_state_t dma_queue_state;
    sci_error_t error;
    unsigned int flags = use_sysdma ? SCI_FLAG_DMA_SYSDMA : NO_FLAGS;

    SCIStartDmaTransferMem(
            *dma_queue,
            rdma_buff,
            *remote_segment,
            sizeof(*rdma_buff),
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

void dma(sci_desc_t v_dev, sci_remote_segment_t remote_segment, bool use_sysdma) {
    sci_dma_queue_t dma_queue;
    sci_error_t error;
    sci_dma_queue_state_t dma_q_state;
    rdma_buff_t rdma_buff;

    rdma_buff.done = 0;
    strcpy(rdma_buff.word, "OK");

    SCICreateDMAQueue(v_dev, &dma_queue, ADAPTER_NO, 1, NO_FLAGS, &error);
    print_sisci_error(&error, "SCICreateDMAQueue", true);

    dma_q_state = SCIDMAQueueState(dma_queue);
    if (dma_q_state == SCI_DMAQUEUE_IDLE)
        printf("DMA queue is idle\n");
    else {
        fprintf(stderr, "DMA queue is not idle\n");
        exit(EXIT_FAILURE);
    }

    send_dma_buff(&dma_queue, &rdma_buff, &remote_segment, use_sysdma);

    rdma_buff.done = 1;
    send_dma_buff(&dma_queue, &rdma_buff, &remote_segment, use_sysdma);

    SCIRemoveDMAQueue(dma_queue, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIRemoveDMAQueue", false);
}