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
#include "dma.h"

void print_usage(char *prog_name) {
    printf("usage: %s [-nid <opt> | -an <opt>] [--use-local-addr] <mode>\n", prog_name);
    printf("    -nid <sender node id>         : Specify the sender using node id\n");
    printf("    -an <sender adapter name>     : Specify the sender using its adapter name\n");
    printf("    --use-local-addr              : Do not create a local segment, use malloc\n");
    printf("    <mode>                        : Mode of operation\n");
    printf("           poll                   : Busy wait for transfer\n");
    printf("           rma                    : Map remote segment, and read from it directly\n");
    printf("           rma-check              : Map remote segment, and read from it directly, then flush and check\n");
    printf("           dma-global             : Use DMA to read provided by the HCA global port\n");

    exit(EXIT_FAILURE);
}

static void poll(sci_desc_t v_dev, unsigned int local_node_id) {
    sci_error_t error;
    rdma_buff_t *rdma_buff;
    segment_local_args_t local = {0};

    local.segment_size = RECEIVER_SEG_SIZE;

    init_local_segment(v_dev, &local, NO_CALLBACK, NO_ARG, RECEIVER_SEG_ID, NO_FLAGS);

    memset(local.address, 0, RECEIVER_SEG_SIZE);

    SCISetSegmentAvailable(local.segment,
                           ADAPTER_NO,
                           NO_FLAGS,
                           &error);
    print_sisci_error(&error, "SCISetSegmentAvailable", true);

    printf("Node %u waiting for transfer\n", local_node_id);
    rdma_buff = (rdma_buff_t *) local.address;
    while (!rdma_buff->done);
    printf("RDMA Done! Word: %s\n", rdma_buff->word);

    SCISetSegmentUnavailable(local.segment, ADAPTER_NO, NO_FLAGS, &error);
    print_sisci_error(&error, "SCISetSegmentUnavailable", false);

    destroy_local_segment(&local, NO_FLAGS);
}

static void rma(sci_desc_t v_dev, unsigned int receiver_id, bool check) {
    volatile rdma_buff_t *rdma_buff;
    sci_sequence_t remote_sequence;
    segment_remote_args_t remote = {0};

    init_remote_connect(v_dev, &remote.segment, receiver_id);

    rma_init(&remote);
    fprintf(stderr, "Remote segment mapped\n");
    if (check) {
        rma_sequence_init(remote.map, &remote_sequence);
        rma_check(remote_sequence);
    }

    rdma_buff = (rdma_buff_t *) remote.address;

    printf("Node waiting for transfer\n");

    printf("Checking done\n");
    while (!rdma_buff->done);
    printf("RDMA Done! Word: %s\n", rdma_buff->word);
    if (check) {
        rma_check(remote_sequence);
        rma_destroy_sequence(remote_sequence);
    }
    rma_destroy(remote.map);
    destroy_remote_connect(remote.segment, NO_FLAGS);
}

int main(int argc, char *argv[]) {
    sci_desc_t v_dev;
    sci_error_t error;
    unsigned int local_node_id;
    unsigned int receiver_id = UNINITIALIZED_ARG;
    unsigned int use_local_addr = UNINITIALIZED_ARG;
    sci_remote_segment_t remote_segment = NULL;
    char *mode;

    if (parse_id_args(argc, argv, &receiver_id, &use_local_addr, print_usage) != argc) print_usage(argv[0]);
    mode = argv[argc-1];
    if (strcmp(mode, "poll") != 0 &&
        strcmp(mode, "dma-global") != 0 &&
        strcmp(mode, "rma") != 0 &&
        strcmp(mode, "rma-check") != 0) print_usage(argv[0]);
    if (strcmp(mode, "poll") != 0 && receiver_id == UNINITIALIZED_ARG) print_usage(argv[0]);

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
        rma(v_dev, receiver_id, false);
    } else if (strcmp(mode, "rma-check") == 0) {
        rma(v_dev, receiver_id, true);
    } else if (strcmp(mode, "dma-global") == 0) {
        init_remote_connect(v_dev, &remote_segment, receiver_id);
        dma_transfer(v_dev, remote_segment, false, true, use_local_addr, true);
        destroy_remote_connect(remote_segment, NO_FLAGS);
    } else {
        print_usage(argv[0]);
    }

    SCIClose(v_dev, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIClose", false);

    SCITerminate();
    printf("SCI termination OK!\n");
    exit(EXIT_SUCCESS);
}
