#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "sisci_api.h"
#include "sisci_error.h"

#include "error_util.h"

#include "rdma_buff.h"
#include "printfo.h"
#include "sisci_glob_defs.h"
#include "rnid_util.h"

#define RECEIVER_SEG_SIZE 4096

void print_usage(char *prog_name) {
    printf("usage: %s [-nid <opt> | -an <opt>] [-chid <opt>] <mode>\n", prog_name);
    printf("    -nid <sender node id>         : Specify the sender using node id\n");         // required later
    printf("    -an <sender adapter name>     : Specify the sender using its adapter name\n");// required later
    printf("    -chid <channel id>            : Specify the DMA channel id, required for mode dma-channel\n");
    printf("    <mode>                        : Mode of operation\n");
    printf("           poll                   : Busy wait for transfer\n");
    printf("           dma-channel            : Use DMA channel to receive data, with busy wait\n");

    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    sci_desc_t v_dev;
    sci_error_t error;
    sci_local_segment_t local_segment;
    sci_map_t local_map;
    rdma_buff_t *rdma_buff;
    unsigned int local_node_id;
    unsigned int receiver_id = -1;
    bool is_channel;
    char *mode;
    sci_dma_channel_t dma_channel;

    if (parse_id_args(argc, argv, &receiver_id, &is_channel, print_usage) != argc) print_usage(argv[0]);
    mode = argv[argc-1];
    if (strcmp(mode, "poll") != 0 && strcmp(mode, "dma-channel") != 0) print_usage(argv[0]);
    if (strcmp(mode, "dma-channel") == 0 && (!is_channel || receiver_id == -1)) print_usage(argv[0]);

    SCIInitialize(NO_FLAGS, &error);
    print_sisci_error(&error, "SCIInitialize", true); 
    printf("SCI initialization OK!\n");

    print_all(ADAPTER_NO);

    SCIGetLocalNodeId(ADAPTER_NO, &local_node_id, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIGetLocalNodeId", true);

    SCIOpen(&v_dev, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIOpen", true);

    SCICreateSegment(v_dev,
            &local_segment,
            RECEIVER_SEG_ID,
            RECEIVER_SEG_SIZE,
            NO_CALLBACK,
            NO_ARG,
            NO_FLAGS,
            &error);
    print_sisci_error(&error, "SCICreateSegment", true);

    if (strcmp(mode, "dma-channel") != 0) {
        SCIPrepareSegment(local_segment,
                          ADAPTER_NO,
                          NO_FLAGS,
                          &error);
        print_sisci_error(&error, "SCIPrepareSegment", true);
    } else {
        SCIRequestDMAChannel(v_dev, &dma_channel, ADAPTER_NO, SCI_DMA_TYPE_SYSTEM, SCI_DMA_CHANNEL_ID_DONTCARE, NO_FLAGS, &error);
        print_sisci_error(&error, "SCIRequestDMAChannel", true);

        SCIPrepareLocalSegmentForDMA(dma_channel, local_segment, NO_FLAGS, &error);
        print_sisci_error(&error, "SCIPrepareLocalSegmentForDMA", true);
    }

    rdma_buff = (rdma_buff_t*) SCIMapLocalSegment(local_segment,
            &local_map,
            NO_OFFSET,
            RECEIVER_SEG_SIZE,
            NO_SUG_ADDR,
            NO_FLAGS,
            &error);
    print_sisci_error(&error, "SCIMapLocalSegment", true);

    memset(rdma_buff, 0, RECEIVER_SEG_SIZE); 

    SCISetSegmentAvailable(local_segment,
        ADAPTER_NO,
        NO_FLAGS,
        &error);
    print_sisci_error(&error, "SCISetSegmentAvailable", true);     

    printf("Node %u waiting for transfer\n", local_node_id);
    while (!rdma_buff->done);
    printf("RDMA Done! Word: %s\n", rdma_buff->word);
    
    SCISetSegmentUnavailable(local_segment, ADAPTER_NO, NO_FLAGS, &error);
    print_sisci_error(&error, "SCISetSegmentUnavailable", false);

    SCIUnmapSegment(local_map, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIUnmapSegment", false);

    SCIRemoveSegment(local_segment, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIRemoveSegment", false);

    SCIClose(v_dev, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIClose", false); 

    SCITerminate();
    printf("SCI termination OK!\n");
    exit(EXIT_SUCCESS);
}
