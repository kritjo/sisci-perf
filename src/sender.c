#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "sisci_api.h"
#include "sisci_error.h"

#include "error_util.h"
#include "printfo.h"

#include "rma.h"
#include "sisci_glob_defs.h"
#include "dma.h"
#include "rnid_util.h"

void print_usage(char *prog_name) {
    printf("usage: %s (-nid <opt> | -an <opt> | -chid <opt>) <mode>\n", prog_name);
    printf("    -nid <receiver node id>       : Specify the receiver using node id\n");
    printf("    -an <receiver adapter name>   : Specify the receiver using its adapter name\n");
    printf("    -chid <channel id>            : Specify the DMA channel id, only has effect for sysdma mode\n");
    printf("    <mode>                        : Mode of operation\n");
    printf("           dma                    : Use DMA from network adapter to remote segment\n");
    printf("           sysdma                 : Use DMA provided by the host system\n");
    printf("           rma                    : Map remote segment, and write to it directly\n");
    printf("           rma-check              : Map remote segment, and write to it directly, then flush and check\n");

    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    sci_desc_t v_dev;
    sci_error_t error;
    sci_remote_segment_t remote_segment;
    size_t remote_segment_size;
    int remote_reachable;
    unsigned int receiver_id = -1;
    unsigned int channel_id = -1;
    char *mode;

    if (parse_id_args(argc, argv, &receiver_id, &channel_id, print_usage) != argc) print_usage(argv[0]);
    if (receiver_id == -1) print_usage(argv[0]);
    mode = argv[argc-1];
    if (strcmp(mode, "dma") != 0 && strcmp(mode, "rma") != 0 && strcmp(mode, "sysdma") != 0 && strcmp(mode, "rma-check") != 0 && strcmp(mode, "dma-channel") != 0) print_usage(argv[0]);

    SCIInitialize(NO_FLAGS, &error);
    print_sisci_error(&error, "SCIInitialize", true); 
    printf("SCI initialization OK!\n");

    print_all(ADAPTER_NO);

    SCIOpen(&v_dev, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIOpen", true); 

    remote_reachable = SCIProbeNode(v_dev, ADAPTER_NO, receiver_id, NO_FLAGS, &error);
    printf("Probe said that receiver node (%d) was", receiver_id);
    if (remote_reachable) printf(" reachable!\n");
    else printf(" NOT reachable!\n");
 
    SCIConnectSegment(v_dev,
                      &remote_segment,
                      receiver_id,
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
