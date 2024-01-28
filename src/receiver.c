#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "sisci_api.h"
#include "sisci_error.h"

#include "error_util.h"

#include "rdma_buff.h"

#define NO_FLAGS 0
#define NO_CALLBACK 0
#define NO_ARG 0

#define ADAPTER_NO 0 // From /etc/dis/dishosts.conf

#define RECEIVER_SEG_ID 1337
#define RECEIVER_SEG_SIZE 4

int main(void) {
    sci_desc_t v_dev;
    sci_error_t error;
    sci_local_segment_t local_segment;
    sci_map_t local_map;
    rdma_buff_t *rdma_buff;

    SCIInitialize(NO_FLAGS, &error);
    print_sisci_error(&error, "SCIInitialize", true); 
    printf("SCI initialization OK!\n");

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
            0,
            RECEIVER_SEG_SIZE,
            0,
            NO_FLAGS,
            &error);
    print_sisci_error(&error, "SCIMapLocalSegment", true);

    memset(rdma_buff, 0, RECEIVER_SEG_SIZE); 

    SCISetSegmentAvailable(local_segment,
        ADAPTER_NO,
        NO_FLAGS,
        &error);
    print_sisci_error(&error, "SCISetSegmentAvailable", true);     

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
