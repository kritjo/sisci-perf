#include "common.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <remote node id> <size>\n", argv[0]);
        exit(1);
    }

    int remote_node_id = atoi(argv[1]);

    printf("Remote node id: %d\n", remote_node_id);

    sci_desc_t sd;
    sci_error_t error;
    void* local_address;
    int curr_counter = 0;

    unsigned int rid, aid;

    sci_local_segment_t segment;
    sci_segment_cb_reason_t reason;

    SEOE(SCIInitialize, NO_FLAGS);
    SEOE(SCIOpen, &sd, NO_FLAGS);

    SEOE(SCICreateSegment, sd, &segment, SEGMENT_ID, SEGMENT_SIZE, NO_CALLBACK, NO_ARGS, NO_FLAGS);
    SEOE(SCIPrepareSegment, segment, ADAPTER_NO, NO_FLAGS);
    SEOE(SCISetSegmentAvailable, segment, ADAPTER_NO, NO_FLAGS);

    do {
        reason = SCIWaitForLocalSegmentEvent(segment, &rid, &aid, SCI_INFINITE_TIMEOUT, NO_FLAGS, &error);
        if (error != SCI_ERR_OK) {
            printf("err waiting: %s\n", SCIGetErrorString(error));
            return 1;
        }
        printf("reason: %d\n", reason);
    } while (reason == SCI_CB_CONNECT || reason == SCI_CB_OPERATIONAL);

    SEOE(SCISetSegmentUnavailable, segment, ADAPTER_NO, NO_FLAGS);
    SEOE(SCIRemoveSegment, segment, ADAPTER_NO);
    SEOE(SCIClose, sd, NO_FLAGS);
    SCITerminate();
    return 0;
}