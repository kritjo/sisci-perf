#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "sisci_api.h"
#include "sisci_error.h"

#include "error_util.h"

#include "rdma_buff.h"

#define NO_FLAGS 0
#define NO_CALLBACK 0
#define NO_ARG 0
#define NO_OFFSET 0

#define ADAPTER_NO 0 // From /etc/dis/dishosts.conf

#define RECEIVER_SEG_ID 1337
#define RECEIVER_NODE_ID 24

void send_rdma_buff(sci_dma_queue_t *dma_queue, rdma_buff_t *rdma_buff, sci_remote_segment_t *remote_segment, sci_error_t *error) {
    sci_dma_queue_state_t dma_queue_state;

    SCIStartDmaTransferMem(
            *dma_queue,
            rdma_buff,
            *remote_segment,
            sizeof(*rdma_buff),
            NO_OFFSET,
            NO_CALLBACK,
            NO_ARG,
            NO_FLAGS,
            error);
    print_sisci_error(error, "SCIStartDmaTransferMem", true);

    SCIWaitForDMAQueue(*dma_queue, SCI_INFINITE_TIMEOUT, NO_FLAGS, error);
    print_sisci_error(error, "SCIWaitForDMAQueue", true);

    dma_queue_state = SCIDMAQueueState(*dma_queue);
    if (dma_queue_state == SCI_DMAQUEUE_DONE) 
        printf("Transfer successful!\n");
    else {
        fprintf(stderr, "Transfer failed!\n");
        exit(EXIT_FAILURE);
    }
}

int main(void) {
    sci_desc_t v_dev;
    sci_error_t error;
    sci_dma_queue_t dma_queue;
    sci_remote_segment_t remote_segment;
    size_t remote_segment_size;
    int remote_reachable;

    rdma_buff_t rdma_buff;
    rdma_buff.done = 0;
    strcpy(rdma_buff.word, "OK");

    SCIInitialize(NO_FLAGS, &error);
    print_sisci_error(&error, "SCIInitialize", true); 
    printf("SCI initialization OK!\n");

    SCIOpen(&v_dev, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIOpen", true); 

    remote_reachable = SCIProbeNode(v_dev, ADAPTER_NO, RECEIVER_NODE_ID, NO_FLAGS, &error);
    printf("Probe said that receiver node (%d) was", RECEIVER_NODE_ID);
    if (remote_reachable) printf(" reachable!\n");
    else printf(" NOT reachable!\n");

    SCICreateDMAQueue(v_dev, &dma_queue, ADAPTER_NO, 1, NO_FLAGS, &error);
    print_sisci_error(&error, "SCICreateDMAQueue", true);    

    SCIConnectSegment(v_dev,
            &remote_segment, 
            RECEIVER_NODE_ID, 
            RECEIVER_SEG_ID, 
            ADAPTER_NO,
            NO_CALLBACK,
            NO_ARG,
            SCI_INFINITE_TIMEOUT,
            NO_FLAGS,
            &error);
    print_sisci_error(&error, "SCIConnectSegment", true);

    remote_segment_size = SCIGetRemoteSegmentSize(remote_segment);
    printf("Connected to remote segment of size %ld\n", remote_segment_size);

    send_rdma_buff(&dma_queue, &rdma_buff, &remote_segment, &error);

    rdma_buff.done = 1;
    send_rdma_buff(&dma_queue, &rdma_buff, &remote_segment, &error);

    SCIRemoveDMAQueue(dma_queue, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIRemoveDMAQueue", false);

    SCIDisconnectSegment(remote_segment, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIDisconnectSegment", false);

    SCIClose(v_dev, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIClose", false); 

    SCITerminate();
    printf("SCI termination OK!\n");
    exit(EXIT_SUCCESS);
}
