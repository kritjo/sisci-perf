#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#include "sisci_api.h"
#include "sisci_error.h"

#include "error_util.h"
#include "printfo.h"

#include "rma.h"
#include "sisci_glob_defs.h"
#include "dma.h"
#include "args_parser.h"
#include "rdma_buff.h"
#include "common.h"
#include "lib_rma.h"

void print_usage(char *prog_name) {
    printf("usage: %s (-nid <opt> | -an <opt>) <mode>\n", prog_name);
    printf("    -nid <receiver node id>       : Specify the receiver using node id\n");
    printf("    -an <receiver adapter name>   : Specify the receiver using its adapter name\n");
    printf("    <mode>                        : Mode of operation\n");
    printf("           dma                    : Use DMA to write (no mode specified)\n");
    printf("           sysdma                 : Use DMA to write provided by the host system\n");
    printf("           rma                    : Map remote segment, and write to it directly\n");
    printf("           rma-check              : Map remote segment, and write to it directly, then flush and check\n");
    printf("           provider               : Provide a segment for the receiver to read\n");

    exit(EXIT_FAILURE);
}


int main(int argc, char *argv[]) {
    sci_desc_t v_dev;
    sci_error_t error;
    sci_remote_segment_t remote_segment = NULL;
    rdma_buff_t *rdma_buff;
    unsigned int receiver_id = UNINITIALIZED_ARG;
    unsigned int local_node_id;
    char *mode;

    if (parse_id_args(argc, argv, &receiver_id, print_usage) != argc) print_usage(argv[0]);
    if (receiver_id == UNINITIALIZED_ARG) print_usage(argv[0]);
    mode = argv[argc-1];
    if (strcmp(mode, "dma") != 0 &&
        strcmp(mode, "rma") != 0 &&
        strcmp(mode, "sysdma") != 0 &&
        strcmp(mode, "rma-check") != 0 &&
        strcmp(mode, "provider") != 0) print_usage(argv[0]);
    SCIInitialize(NO_FLAGS, &error);
    print_sisci_error(&error, "SCIInitialize", true);

    DEBUG_PRINT("SCI initialization OK!\n");
    print_all(ADAPTER_NO);

    SCIOpen(&v_dev, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIOpen", true);

    SCIGetLocalNodeId(ADAPTER_NO, &local_node_id, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIGetLocalNodeId", true);

    if (strcmp(mode, "dma") == 0) {

        remote_connect_init(v_dev, &remote_segment, receiver_id);
        dma_send_test(v_dev, remote_segment, false);
    }
    else if (strcmp(mode, "sysdma") == 0) {
        fprintf(stderr, "SYSDMA is experimental!\n");
        remote_connect_init(v_dev, &remote_segment, receiver_id);
        dma_send_test(v_dev, remote_segment, true);
    }
    else if (strcmp(mode, "rma") == 0) {
        printf("Connecting to remote segment, currently at %p\n", remote_segment);
        remote_connect_init(v_dev, &remote_segment, receiver_id);
        printf("Connecting to remote segment, currently at %p\n", remote_segment);
        rma(remote_segment, false);
    }
    else if (strcmp(mode, "rma-check") == 0) {
        remote_connect_init(v_dev, &remote_segment, receiver_id);
        rma(remote_segment, true);
    }
    else if (strcmp(mode, "provider") == 0) {
        segment_args_t sa;
        sa.local.segment_size = RECEIVER_SEG_SIZE;
        sa.callback_args.callback = NO_CALLBACK;
        sa.callback_args.arg = NO_ARG;

        local_segment_init(v_dev, &sa, NO_FLAGS);

        memset(sa.local.address, 0, RECEIVER_SEG_SIZE);

        rdma_buff = (rdma_buff_t *) sa.local.address;

        rdma_buff->done = 0;
        strcpy(rdma_buff->word, "OK");

        SCISetSegmentAvailable(sa.local.segment,
                               ADAPTER_NO,
                               NO_FLAGS,
                               &error);
        print_sisci_error(&error, "SCISetSegmentAvailable", true);

        sleep(10);

        rdma_buff->done = 1;

        while (1) {
            sleep(1);
        }
    }
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
