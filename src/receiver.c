#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "sisci_api.h"
#include "sisci_error.h"

#include "error_util.h"

#define NO_FLAGS 0
#define NO_CALLBACK 0
#define NO_ARG 0

#define ADAPTER_NO 0 // From /etc/dis/dishosts.conf

//#define SENDER_NODE_ID
//#define SENDER_SEG_ID
//#define SENDER_SEG_SIZE

#define RECEIVER_SEG_ID 1337
//#define RECEIVER_NODE_ID
#define RECEIVER_SEG_SIZE 4

int main(void) {
    sci_desc_t v_dev;
    sci_error_t error;
    sci_local_segment_t local_segment;

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

    SCISetSegmentAvailable(local_segment,
        ADAPTER_NO,
        NO_FLAGS,
        &error);
    print_sisci_error(&error, "SCISetSegmentAvailable", true);



    SCIRemoveSegment(local_segment, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIRemoveSegment", true);

    SCIClose(v_dev, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIClose", true); 

    SCITerminate();
    printf("SCI termination OK!\n");
    return EXIT_SUCCESS;
}
