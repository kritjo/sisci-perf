#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

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
#include "rma.h"

void print_usage(char *prog_name) {
    printf("usage: %s [-nid <opt> | -an <opt> | --multicast] [--use-local-addr] <mode>\n", prog_name);
    printf("    -nid <sender node id>         : Specify the sender using node id\n");
    printf("    -an <sender adapter name>     : Specify the sender using its adapter name\n");
    printf("    --multicast                   : Reflective memory\n");
    printf("    --use-local-addr              : Do not create a local segment, use malloc\n");
    printf("    --request-channel             : Request a DMA channel\n");
    printf("    <mode>                        : Mode of operation\n");
    printf("           poll                   : Busy wait for transfer\n");
    printf("           rma                    : Map remote segment, and read from it directly\n");
    printf("           rma-check              : Map remote segment, and read from it directly, then flush and check\n");
    printf("           dma-global             : Use DMA to read provided by the HCA global port\n");
    printf("           int-provider           : Provide an interrupt for the sender to wait for\n");
    printf("           dint-provider          : Provide a data interrupt for the sender to wait for\n");

    exit(EXIT_FAILURE);
}

static void poll(sci_desc_t v_dev, unsigned int local_node_id, bool multicast) {
    sci_error_t error;
    rdma_buff_t *rdma_buff;
    segment_local_args_t local = {0};

    local.segment_size = RECEIVER_SEG_SIZE;

    init_local_segment(v_dev, &local, NO_CALLBACK, RECEIVER_SEG_ID, false, multicast ? SCI_FLAG_BROADCAST : NO_FLAGS, NO_FLAGS);

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

    if (multicast) {
        while (1) {
            sleep(1);
            printf("Word: %s\n", rdma_buff->word);
        }
    }

    SCISetSegmentUnavailable(local.segment, ADAPTER_NO, NO_FLAGS, &error);
    print_sisci_error(&error, "SCISetSegmentUnavailable", false);

    destroy_local_segment(&local);
}

static sci_callback_action_t data_interrupt(void *_arg,
                                            sci_local_data_interrupt_t _interrupt,
                                            void *data,
                                            unsigned int length,
                                            sci_error_t _status) {
    printf("Data interrupt: ");
    for (unsigned int i = 0; i < length; i++) {
        printf("%c", ((char *) data)[i]);
    }
    printf("\n");
    return SCI_CALLBACK_CONTINUE;
}

int main(int argc, char *argv[]) {
    sci_desc_t v_dev;
    sci_error_t error;
    unsigned int local_node_id;
    unsigned int receiver_id = UNINITIALIZED_ARG;
    unsigned int use_local_addr = UNINITIALIZED_ARG;
    unsigned int req_chnl = UNINITIALIZED_ARG;
    bool multicast = false;
    sci_remote_segment_t remote_segment = NULL;
    char *mode;

    if (parse_id_args(argc, argv, &receiver_id, &use_local_addr, &req_chnl, print_usage) != argc) print_usage(argv[0]);
    mode = argv[argc-1];
    if (receiver_id == DIS_BROADCAST_NODEID_GROUP_ALL) multicast = true;
    if (strcmp(mode, "poll") != 0 &&
        strcmp(mode, "dma-global") != 0 &&
        strcmp(mode, "rma") != 0 &&
        strcmp(mode, "rma-check") != 0 &&
        strcmp(mode, "int-provider") != 0 &&
        strcmp(mode, "dint-provider") != 0) print_usage(argv[0]);
    if ((
            strcmp(mode, "dma-global") == 0 ||
            strcmp(mode, "rma") == 0 ||
            strcmp(mode, "rma-check") == 0
        ) &&
        receiver_id == UNINITIALIZED_ARG) print_usage(argv[0]);

    SCIInitialize(NO_FLAGS, &error);
    print_sisci_error(&error, "SCIInitialize", true);
    DEBUG_PRINT("SCI initialization OK!\n");

    print_all(ADAPTER_NO);

    SCIGetLocalNodeId(ADAPTER_NO, &local_node_id, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIGetLocalNodeId", true);

    SCIOpen(&v_dev, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIOpen", true);

    if (strcmp(mode, "poll") == 0) {
        poll(v_dev, local_node_id, multicast);
    } else if (strcmp(mode, "rma") == 0) {
        init_remote_connect(v_dev, &remote_segment, receiver_id, multicast ? SCI_FLAG_BROADCAST : NO_FLAGS);
        rma(remote_segment, false, false);
        destroy_remote_connect(remote_segment);
    } else if (strcmp(mode, "rma-check") == 0) {
        init_remote_connect(v_dev, &remote_segment, receiver_id, multicast ? SCI_FLAG_BROADCAST : NO_FLAGS);
        rma(remote_segment, true, false);
        destroy_remote_connect(remote_segment);
    } else if (strcmp(mode, "dma-global") == 0) {
        init_remote_connect(v_dev, &remote_segment, receiver_id, multicast ? SCI_FLAG_BROADCAST : NO_FLAGS);
        dma_transfer(v_dev, remote_segment, false, true, use_local_addr != UNINITIALIZED_ARG, false, req_chnl != UNINITIALIZED_ARG, multicast);
        destroy_remote_connect(remote_segment);
    } else if (strcmp(mode, "int-provider") == 0) {
        sci_local_interrupt_t local_interrupt;
        unsigned int interrupt_no = 0;

        SCICreateInterrupt(v_dev,
                           &local_interrupt,
                           ADAPTER_NO,
                           &interrupt_no,
                           NO_CALLBACK,
                           NO_ARG,
                           SCI_FLAG_FIXED_INTNO,
                           &error);
        print_sisci_error(&error, "SCICreateInterrupt", true);

        printf("Waiting for interrupt with number %u\n", interrupt_no);

        SCIWaitForInterrupt(local_interrupt, UINT32_MAX, NO_FLAGS, &error);
        print_sisci_error(&error, "SCIWaitForInterrupt", true);

        SCIRemoveInterrupt(local_interrupt, NO_FLAGS, &error);
    } else if (strcmp(mode, "dint-provider") == 0) {
        sci_local_data_interrupt_t local_interrupt;
        unsigned int interrupt_no = 1;

        SCICreateDataInterrupt(v_dev,
                               &local_interrupt,
                               ADAPTER_NO,
                               &interrupt_no,
                               data_interrupt,
                               NO_ARG,
                               SCI_FLAG_USE_CALLBACK | SCI_FLAG_FIXED_INTNO,
                               &error);
        print_sisci_error(&error, "SCICreateDataInterrupt", true);

        printf("Waiting for data interrupt with number %u\n", interrupt_no);

        sleep(60);

        SCIRemoveDataInterrupt(local_interrupt, NO_FLAGS, &error);
    } else {
        print_usage(argv[0]);
    }

    SCIClose(v_dev, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIClose", false);

    SCITerminate();
    printf("SCI termination OK!\n");
    exit(EXIT_SUCCESS);
}
