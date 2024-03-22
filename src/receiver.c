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
#include "lib_rma.h"

void print_usage(char *prog_name) {
    printf("usage: %s [-nid <opt> | -an <opt>] [-chid <opt>] <mode>\n", prog_name);
    printf("    -nid <sender node id>         : Specify the sender using node id\n");
    printf("    -an <sender adapter name>     : Specify the sender using its adapter name\n");
    printf("    <mode>                        : Mode of operation\n");
    printf("           poll                   : Busy wait for transfer\n");
    printf("           rma                    : Map remote segment, and read from it directly\n");

    exit(EXIT_FAILURE);
}

static void poll(sci_desc_t v_dev, unsigned int local_node_id) {
    sci_error_t error;
    rdma_buff_t *rdma_buff;

    segment_args_t sa;
    sa.local.segment_size = RECEIVER_SEG_SIZE;
    sa.callback_args.callback = NO_CALLBACK;
    sa.callback_args.arg = NO_ARG;

    local_segment_init(v_dev, &sa, NO_FLAGS);

    memset(sa.local.address, 0, RECEIVER_SEG_SIZE);

    SCISetSegmentAvailable(sa.local.segment,
                           ADAPTER_NO,
                           NO_FLAGS,
                           &error);
    print_sisci_error(&error, "SCISetSegmentAvailable", true);

    printf("Node %u waiting for transfer\n", local_node_id);
    rdma_buff = (rdma_buff_t *) sa.local.address;
    while (!rdma_buff->done);
    printf("RDMA Done! Word: %s\n", rdma_buff->word);

    SCISetSegmentUnavailable(sa.local.segment, ADAPTER_NO, NO_FLAGS, &error);
    print_sisci_error(&error, "SCISetSegmentUnavailable", false);

    SCIUnmapSegment(sa.local.map, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIUnmapSegment", false);

    SCIRemoveSegment(sa.local.segment, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIRemoveSegment", false);
}

static void rma(sci_desc_t v_dev, unsigned int receiver_id) {
    sci_map_t remote_map;
    volatile rdma_buff_t *rdma_buff;
    sci_remote_segment_t remote_segment = NULL;
    sci_sequence_t remote_sequence;

    remote_connect_init(v_dev, &remote_segment, receiver_id);

    rma_init(remote_segment, (volatile void **) &rdma_buff, &remote_map);
    fprintf(stderr, "Remote segment mapped\n");
    rma_sequence_init(remote_map, &remote_sequence);
    rma_check(remote_sequence);

    printf("Node waiting for transfer\n");

    printf("Checking done\n");
    while (!rdma_buff->done);
    printf("RDMA Done! Word: %s\n", rdma_buff->word);
    rma_destroy_sequence(remote_sequence);
    rma_destroy(remote_map);
}

int main(int argc, char *argv[]) {
    sci_desc_t v_dev;
    sci_error_t error;
    unsigned int local_node_id;
    unsigned int receiver_id = UNINITIALIZED_ARG;
    char *mode;

    if (parse_id_args(argc, argv, &receiver_id, print_usage) != argc) print_usage(argv[0]);
    mode = argv[argc-1];
    if (strcmp(mode, "poll") != 0 &&
        strcmp(mode, "rma") != 0) print_usage(argv[0]);
    if (strcmp(mode, "rma") == 0 && receiver_id == UNINITIALIZED_ARG) print_usage(argv[0]);

    SCIInitialize(NO_FLAGS, &error);
    print_sisci_error(&error, "SCIInitialize", true);
    DEBUG_PRINT("SCI initialization OK!\n");

    print_all(ADAPTER_NO);

    SCIGetLocalNodeId(ADAPTER_NO, &local_node_id, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIGetLocalNodeId", true);

    SCIOpen(&v_dev, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIOpen", true);

    if (strcmp(mode, "poll") == 0) {
        poll(v_dev, local_node_id);
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
