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
#include "args_parser.h"
#include "common.h"

void print_usage(char *prog_name) {
    printf("usage: %s [-nid <opt> | -an <opt>] [-chid <opt>] <mode>\n", prog_name);
    printf("    -nid <sender node id>         : Specify the sender using node id\n");         // required later
    printf("    -an <sender adapter name>     : Specify the sender using its adapter name\n");// required later
    //printf("    -chid <channel id>            : Specify the DMA channel id, required for mode dma-channel\n");
    printf("    <mode>                        : Mode of operation\n");
    printf("           poll                   : Busy wait for transfer\n");
    printf("           rma                    : Map remote segment, and read from it directly\n");

    exit(EXIT_FAILURE);
}

static void poll(sci_desc_t v_dev, unsigned int local_node_id, unsigned int receiver_id, unsigned int channel_id) {
    sci_error_t error;
    sci_local_segment_t local_segment;
    sci_map_t local_map;
    rdma_buff_t *rdma_buff;
    sci_dma_channel_t dma_channel;

    SCICreateSegment(v_dev,
                     &local_segment,
                     RECEIVER_SEG_ID,
                     RECEIVER_SEG_SIZE,
                     NO_CALLBACK,
                     NO_ARG,
                     NO_FLAGS,
                     &error);
    print_sisci_error(&error, "SCICreateSegment", true);

    //if (channel_id == UNINITIALIZED_ARG) {
        SCIPrepareSegment(local_segment,
                          ADAPTER_NO,
                          NO_FLAGS,
                          &error);
        print_sisci_error(&error, "SCIPrepareSegment", true);
    //} else {
    //    SCIRequestDMAChannel(v_dev, &dma_channel, ADAPTER_NO, SCI_DMA_TYPE_SYSTEM, channel_id, NO_FLAGS, &error);
    //    print_sisci_error(&error, "SCIRequestDMAChannel", true);

    //    SCIPrepareLocalSegmentForDMA(dma_channel, local_segment, NO_FLAGS, &error);
    //   print_sisci_error(&error, "SCIPrepareLocalSegmentForDMA", true);
    //}

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
}

static void rma(sci_desc_t v_dev, unsigned int receiver_id) {
    sci_error_t error = SCI_ERR_OK;
    sci_map_t remote_map;
    rdma_buff_t *rdma_buff;
    size_t remote_segment_size = 0;
    sci_remote_segment_t remote_segment = NULL;

    remote_connect_init(v_dev, &remote_segment, remote_segment_size, receiver_id, error);

    rdma_buff = (rdma_buff_t*) SCIMapRemoteSegment(remote_segment,
                                                  &remote_map,
                                                  NO_OFFSET,
                                                  RECEIVER_SEG_SIZE,
                                                  NO_SUG_ADDR,
                                                  NO_FLAGS,
                                                  &error);
    print_sisci_error(&error, "SCIMapRemoteSegment", true);

    printf("Node waiting for transfer\n");

    while (!rdma_buff->done);
    printf("RDMA Done!\n");

    SCIUnmapSegment(remote_map, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIUnmapSegment", false);
}

int main(int argc, char *argv[]) {
    sci_desc_t v_dev;
    sci_error_t error;
    unsigned int local_node_id;
    unsigned int receiver_id = UNINITIALIZED_ARG;
    unsigned int channel_id = UNINITIALIZED_ARG;
    char *mode;

    if (parse_id_args(argc, argv, &receiver_id, &channel_id, print_usage) != argc) print_usage(argv[0]);
    mode = argv[argc-1];
    if (strcmp(mode, "poll") != 0 &&
        strcmp(mode, "rma") != 0) print_usage(argv[0]);

    SCIInitialize(NO_FLAGS, &error);
    print_sisci_error(&error, "SCIInitialize", true);
    DEBUG_PRINT("SCI initialization OK!\n");

    print_all(ADAPTER_NO);

    SCIGetLocalNodeId(ADAPTER_NO, &local_node_id, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIGetLocalNodeId", true);

    SCIOpen(&v_dev, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIOpen", true);

    if (strcmp(mode, "poll") == 0) {
        poll(v_dev, local_node_id, receiver_id, channel_id);
    } else if (strcmp(mode, "rma") == 0) {
        rma(v_dev, receiver_id);
    } else {
        print_usage(argv[0]);
    }

    SCIClose(v_dev, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIClose", false);

    SCITerminate();
    printf("SCI termination OK!\n");
    exit(EXIT_SUCCESS);
}
