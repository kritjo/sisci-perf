#include "common.h"

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <remote node id> <size>\n", argv[0]);
        exit(1);
    }

    int remote_node_id = atoi(argv[1]);
    int size = atoi(argv[2]);

    printf("Remote node id: %d\n", remote_node_id);
    printf("Size: %d\n", size);

    sci_desc_t sd;
    sci_error_t error;
    void* local_address;
    int curr_counter = 0;

    sci_local_segment_t segment;
    sci_segment_cb_reason_t reason;

    SEOE(SCIInitialize, NO_FLAGS);
    SEOE(SCIOpen, &sd, NO_FLAGS);

    SEOE(SCICreateSegment, sd, &segment, SEGMENT_ID, SEGMENT_SIZE, NO_CALLBACK, NO_ARGS, NO_FLAGS);
    SEOE(SCIPrepareSegment, segment, ADAPTER_NO, NO_FLAGS);
    SEOE(SCISetSegmentAvailable, segment, ADAPTER_NO, NO_FLAGS);
    
    do {
        reason = SCIWaitForLocalSegmentEvent(segment, &remote_node_id, ADAPTER_NO, SCI_INFINITE_TIMEOUT, NO_FLAGS, &error);
        if (error != SCI_ERR_OK) return 1;
    } while (reason != SCI_CB_CONNECT && reason != SCI_CB_OPERATIONAL);

    SEOE(SCISetSegmentUnavailable, ADAPTER_NO, segment, NO_FLAGS);
    SEOE(SCIRemoveSegment, ADAPTER_NO, segment);
    SEOE(SCIClose, sd, NO_FLAGS);
    SCITerminate();
    return 0;
}