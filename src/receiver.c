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

#define ADAPTER_NO 0 // From /etc/dis/dishosts.conf

#define RECEIVER_SEG_ID 1337
#define RECEIVER_SEG_SIZE 4096

int main(void) {
    sci_desc_t v_dev;
    sci_error_t error;
    sci_local_segment_t local_segment;
    sci_map_t local_map;
    rdma_buff_t *rdma_buff;
    unsigned int local_node_id;

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

    SCIPrepareSegment(local_segment,
            ADAPTER_NO,
            NO_FLAGS,
            &error);
    print_sisci_error(&error, "SCIPrepareSegment", true);

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
