#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

#include "sisci_api.h"
#include "sisci_error.h"

#include "error_util.h"
#include "printfo.h"

#include "rdma_buff.h"

#define NO_FLAGS 0
#define NO_CALLBACK 0
#define NO_ARG 0
#define NO_OFFSET 0

#define ADAPTER_NO 0 // From /etc/dis/dishosts.conf

#define RECEIVER_SEG_ID 1337

void print_usage(char *prog_name) {
    printf("usage: %s (-nid <opt> | -an <opt>) <mode>\n", prog_name);
    printf("    -nid <receiver node id>       : Specify the receiver using node id\n");
    printf("    -an <receiver adapter name>   : Specify the receiver using its adapter name\n");
    printf("    <mode>                        : Mode of operation, either 'dma' or 'rma'\n");
    printf("           dma                    : Use DMA from network adapter to remote segment\n");
    printf("           sysdma                 : Use DMA provided by the host system\n");
    printf("           rma                    : Map remote segment, and write to it directly\n");

    exit(EXIT_FAILURE);
}

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

void rma(sci_remote_segment_t remote_segment) {
    volatile rdma_buff_t *remote_address;
    sci_map_t remote_map;
    sci_error_t error;
    size_t remote_segment_size;
    rdma_buff_t rdma_buff;
    rdma_buff.done = 0;
    strcpy(rdma_buff.word, "OK");

    remote_segment_size = SCIGetRemoteSegmentSize(remote_segment);

    remote_address = (volatile rdma_buff_t*)SCIMapRemoteSegment(
            remote_segment, &remote_map,0 /* offset */, remote_segment_size,
            0 /* address hint */, NO_FLAGS, &error);

    print_sisci_error(&error, "SCIMapRemoteSegment", true);

    *remote_address = rdma_buff;
    remote_address->done = 1;
    printf("Wrote to remote segment\n");

    SCIUnmapSegment(remote_map, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIUnmapSegment", false);
}

bool parse_nid(char *arg, unsigned int *receiver_node_id) {
    long strtol_tmp;
    char *endptr;

    errno = 0;
    
    strtol_tmp = strtol(arg, &endptr, 10);
    if (errno != 0) {
        perror("strtol");
        exit(EXIT_FAILURE);
    }

    if (strtol_tmp > UINT_MAX || strtol_tmp < 0) return false;
    if (*endptr != '\0') return false;
    
    *receiver_node_id = (unsigned int) strtol_tmp;
    return true;
}

bool parse_an(char *arg, unsigned int *receiver_node_id) { 
    dis_virt_adapter_t adapter_type;
    sci_error_t error;
    dis_nodeId_list_t nodelist;

    SCIGetNodeIdByAdapterName(arg, &nodelist, &adapter_type, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIGetNodeIdByAdapterName", true);

    if (nodelist[0] != 0) {
        printf("Found node with id %u matching adapter name\n", nodelist[0]);
        *receiver_node_id = nodelist[0];
    } else {
        printf("No matching adapters found matching %s\n", arg);
        return false;
    }

    if (nodelist[1] != 0) printf("    (multiple adapters found)\n");
    return true;
}

int parseRnidArgs(int argc, char *argv[], unsigned int *receiver_node_id) {
    bool rnid_set = false;
    int arg;

    for (arg = 0; arg < argc; arg++) {
        if (strcmp(argv[arg], "-nid") == 0) {
            if (rnid_set) print_usage(argv[0]);
            if (arg+1 == argc) print_usage(argv[0]);
            rnid_set = parse_nid(argv[arg+1], receiver_node_id);
            if (!rnid_set) print_usage(argv[0]);
        }
        else if (strcmp(argv[arg], "-an") == 0) {
           if (rnid_set) print_usage(argv[0]);
           if (arg+1 == argc) print_usage(argv[0]);
           rnid_set = parse_an(argv[arg+1], receiver_node_id);
           if (!rnid_set) print_usage(argv[0]);
        }
    }

    if (!rnid_set) print_usage(argv[0]);
    return arg;
}

int main(int argc, char *argv[]) {
    sci_desc_t v_dev;
    sci_error_t error;
    sci_remote_segment_t remote_segment;
    size_t remote_segment_size;
    int remote_reachable;
    unsigned int receiver_node_id;
    char *mode;

    if (parseRnidArgs(argc, argv, &receiver_node_id) != argc) print_usage(argv[0]);
    mode = argv[argc-1];
    if (strcmp(mode, "dma") != 0 && strcmp(mode, "rma") != 0 && strcmp(mode, "sysdma") != 0) print_usage(argv[0]);

    SCIInitialize(NO_FLAGS, &error);
    print_sisci_error(&error, "SCIInitialize", true); 
    printf("SCI initialization OK!\n");

    print_all(ADAPTER_NO);

    SCIOpen(&v_dev, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIOpen", true); 

    remote_reachable = SCIProbeNode(v_dev, ADAPTER_NO, receiver_node_id, NO_FLAGS, &error);
    printf("Probe said that receiver node (%d) was", receiver_node_id);
    if (remote_reachable) printf(" reachable!\n");
    else printf(" NOT reachable!\n");
 
    SCIConnectSegment(v_dev,
            &remote_segment, 
            receiver_node_id, 
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

    if (strcmp(mode, "dma") == 0) dma(v_dev, remote_segment, 0);
    else if (strcmp(mode, "sysdma") == 0) dma(v_dev, remote_segment, 1);
    else if (strcmp(mode, "rma") == 0) rma(remote_segment);
    else {
        fprintf(stderr, "Invalid mode\n");
        exit(EXIT_FAILURE);
    }

    SCIDisconnectSegment(remote_segment, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIDisconnectSegment", false);

    SCIClose(v_dev, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIClose", false); 

    SCITerminate();
    printf("SCI termination OK!\n");
    exit(EXIT_SUCCESS);
}
