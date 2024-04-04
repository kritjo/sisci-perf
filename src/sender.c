#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
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
    printf("usage: %s (-nid <opt> | -an <opt>) [--use-local-addr] <mode>\n", prog_name);
    printf("    -nid <receiver node id>       : Specify the receiver using node id\n");
    printf("    -an <receiver adapter name>   : Specify the receiver using its adapter name\n");
    printf("    --use-local-addr              : Do not create a local segment, use malloc\n");
    printf("    --request-channel             : Request a DMA channel\n");
    printf("    <mode>                        : Mode of operation\n");
    printf("           dma-any                : Use DMA to write (no mode specified)\n");
    printf("           dma-sys                : Use DMA to write provided by the host system\n");
    printf("           dma-global             : Use DMA to write provided by the HCA global port\n");
    printf("           rma                    : Map remote segment, and write to it directly\n");
    printf("           rma-check              : Map remote segment, and write to it directly, then flush and check\n");
    printf("           provider               : Provide a segment for the receiver to read\n");
    printf("           int                    : Invoke an interrupt\n");
    printf("           dint                   : Invoke a data interrupt\n");

    exit(EXIT_FAILURE);
}


int main(int argc, char *argv[]) {
    sci_desc_t v_dev;
    sci_error_t error;
    sci_remote_segment_t remote_segment = NULL;
    rdma_buff_t *rdma_buff;
    unsigned int receiver_id = UNINITIALIZED_ARG;
    unsigned int use_local_addr = UNINITIALIZED_ARG;
    unsigned int req_chnl = UNINITIALIZED_ARG;
    unsigned int local_node_id;
    char *mode;

    if (parse_id_args(argc, argv, &receiver_id, &use_local_addr, &req_chnl, print_usage) != argc) print_usage(argv[0]);
    if (receiver_id == UNINITIALIZED_ARG) print_usage(argv[0]);
    mode = argv[argc-1];
    if (strcmp(mode, "dma-any")    != 0 &&
        strcmp(mode, "dma-sys")    != 0 &&
        strcmp(mode, "dma-global") != 0 &&
        strcmp(mode, "rma")        != 0 &&
        strcmp(mode, "rma-check")  != 0 &&
        strcmp(mode, "provider")   != 0 &&
        strcmp(mode, "int")        != 0 &&
        strcmp(mode, "dint")       != 0) print_usage(argv[0]);

    SCIInitialize(NO_FLAGS, &error);
    print_sisci_error(&error, "SCIInitialize", true);

    DEBUG_PRINT("SCI initialization OK!\n");
    print_all(ADAPTER_NO);

    SCIOpen(&v_dev, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIOpen", true);

    SCIGetLocalNodeId(ADAPTER_NO, &local_node_id, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIGetLocalNodeId", true);

    bool use_sysdma = strcmp(mode, "dma-sys") == 0;
    bool use_globdma = strcmp(mode, "dma-global") == 0;

    if (strcmp(mode, "dma-any") == 0 || use_sysdma || use_globdma) {
        init_remote_connect(v_dev, &remote_segment, receiver_id);
        dma_transfer(v_dev, remote_segment, use_sysdma, use_globdma, use_local_addr, true, req_chnl);
        destroy_remote_connect(remote_segment);
    }
    else if (strcmp(mode, "dma-sys") == 0) {
        fprintf(stderr, "SYSDMA is experimental!\n");
        init_remote_connect(v_dev, &remote_segment, receiver_id);
        dma_transfer(v_dev, remote_segment, true, false, use_local_addr, true, req_chnl);
        destroy_remote_connect(remote_segment);
    }
    else if (strcmp(mode, "dma-global") == 0) {
        init_remote_connect(v_dev, &remote_segment, receiver_id);
        dma_transfer(v_dev, remote_segment, false, true, use_local_addr, true, req_chnl);
        destroy_remote_connect(remote_segment);
    }
    else if (strcmp(mode, "rma") == 0) {
        init_remote_connect(v_dev, &remote_segment, receiver_id);
        rma(remote_segment, false, true);
        destroy_remote_connect(remote_segment);
    }
    else if (strcmp(mode, "rma-check") == 0) {
        init_remote_connect(v_dev, &remote_segment, receiver_id);
        rma(remote_segment, true, true);
        destroy_remote_connect(remote_segment);
    }
    else if (strcmp(mode, "provider") == 0) {
        segment_local_args_t local = {0};
        local.segment_size = RECEIVER_SEG_SIZE;

        init_local_segment(v_dev, &local, NO_CALLBACK, RECEIVER_SEG_ID, false, NO_FLAGS, NO_FLAGS);

        memset(local.address, 0, RECEIVER_SEG_SIZE);

        rdma_buff = (rdma_buff_t *) local.address;

        rdma_buff->done = 0;
        strcpy(rdma_buff->word, "OK");
        rdma_buff->done = 1;

        SCISetSegmentAvailable(local.segment,
                               ADAPTER_NO,
                               NO_FLAGS,
                               &error);
        print_sisci_error(&error, "SCISetSegmentAvailable", true);

        while (1) {
            sleep(1);
        }

        destroy_local_segment(&local);
    }
    else if (strcmp(mode, "int") == 0) {
        sci_remote_interrupt_t remote_interrupt = NULL;

        SCIConnectInterrupt(v_dev, &remote_interrupt, receiver_id, ADAPTER_NO, 0, SCI_INFINITE_TIMEOUT, NO_FLAGS, &error);
        print_sisci_error(&error, "SCIConnectInterrupt", true);

        SCITriggerInterrupt(remote_interrupt, NO_FLAGS, &error);
        print_sisci_error(&error, "SCITriggerInterrupt", true);

        SCIDisconnectInterrupt(remote_interrupt, NO_FLAGS, &error);
        print_sisci_error(&error, "SCIDisconnectInterrupt", false);

        printf("Interrupt sent!\n");
    }
    else if (strcmp(mode, "dint") == 0) {
        sci_remote_data_interrupt_t remote_data_interrupt = NULL;

        SCIConnectDataInterrupt(v_dev, &remote_data_interrupt, receiver_id, ADAPTER_NO, 1, SCI_INFINITE_TIMEOUT, NO_FLAGS, &error);
        print_sisci_error(&error, "SCIConnectDataInterrupt", true);

        char data[] = "OK!";

        SCITriggerDataInterrupt(remote_data_interrupt, data, sizeof(data), NO_FLAGS, &error);
        print_sisci_error(&error, "SCITriggerDataInterrupt", true);

        SCIDisconnectDataInterrupt(remote_data_interrupt, NO_FLAGS, &error);
        print_sisci_error(&error, "SCIDisconnectDataInterrupt", false);

        printf("Data interrupt sent!\n");
    }
    else {
        fprintf(stderr, "Invalid mode\n");
        exit(EXIT_FAILURE);
    }

    SCIClose(v_dev, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIClose", false);

    SCITerminate();
    printf("SCI termination OK!\n");
    exit(EXIT_SUCCESS);
}
