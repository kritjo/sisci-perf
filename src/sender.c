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
#include "rma.h"
#include "sisci_glob_defs.h"

void print_usage(char *prog_name) {
    printf("usage: %s (-nid <opt> | -an <opt>) <mode>\n", prog_name);
    printf("    -nid <receiver node id>       : Specify the receiver using node id\n");
    printf("    -an <receiver adapter name>   : Specify the receiver using its adapter name\n");
    printf("    <mode>                        : Mode of operation, either 'dma' or 'rma'\n");
    printf("           dma                    : Use DMA from network adapter to remote segment\n");
    printf("           sysdma                 : Use DMA provided by the host system\n");
    printf("           rma                    : Map remote segment, and write to it directly\n");
    printf("           rma-check              : Map remote segment, and write to it directly, then flush and check\n");

    exit(EXIT_FAILURE);
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

int parse_rnid_args(int argc, char *argv[], unsigned int *receiver_node_id) {
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

    if (parse_rnid_args(argc, argv, &receiver_node_id) != argc) print_usage(argv[0]);
    mode = argv[argc-1];
    if (strcmp(mode, "dma") != 0 && strcmp(mode, "rma") != 0 && strcmp(mode, "sysdma") != 0 && strcmp(mode, "rma-check") != 0) print_usage(argv[0]);

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

    if (strcmp(mode, "dma") == 0) dma(v_dev, remote_segment, false);
    else if (strcmp(mode, "sysdma") == 0) dma(v_dev, remote_segment, true);
    else if (strcmp(mode, "rma") == 0) rma(remote_segment, false);
    else if (strcmp(mode, "rma-check") == 0) rma(remote_segment, true);
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
