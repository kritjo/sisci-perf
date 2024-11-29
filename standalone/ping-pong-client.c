#include "ping-pong-common.h"
#include <time.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <remote node id> <size of 'read'>\n", argv[0]);
        exit(1);
    }

    int remote_node_id = atoi(argv[1]);
    int size = atoi(argv[2]);

    printf("Remote node id: %d\n", remote_node_id);
    printf("Size of 'read': %d\n", size);

    sci_desc_t sd;
    sci_local_segment_t segment;
    sci_error_t error;
    sci_map_t local_map;
    sci_sequence_t sequence;
    ping_pong_segment_t* local_address;
    int curr_counter = 0;

    sci_remote_segment_t remote_segment;
    sci_map_t remote_map;
    volatile int* remote_address;

    SEOE(SCIInitialize, NO_FLAGS);
    SEOE(SCIOpen, &sd, NO_FLAGS);

    SEOE(SCICreateSegment, sd, &segment, CLIENT_SEGMENT_ID, SEGMENT_SIZE, NO_CALLBACK, NO_ARGS, NO_FLAGS);
    SEOE(SCIPrepareSegment, segment, ADAPTER_NO, NO_FLAGS);
    SEOE(SCISetSegmentAvailable, segment, ADAPTER_NO, NO_FLAGS);

    /* map the local segment */
    local_address = (ping_pong_segment_t*) SCIMapLocalSegment(segment, &local_map, NO_OFFSET, SEGMENT_SIZE, NO_ADDRESS_HINT, NO_FLAGS, &error);
    if (error != SCI_ERR_OK) return 1;

    local_address->counter = 0;

    SEOE(SCIConnectSegment, sd, &remote_segment, remote_node_id, SERVER_SEGMENT_ID, ADAPTER_NO, NO_CALLBACK, NO_ARGS, SCI_INFINITE_TIMEOUT, NO_FLAGS);
    remote_address = (volatile int*) SCIMapRemoteSegment(remote_segment, &remote_map, NO_OFFSET, SEGMENT_SIZE, NO_ADDRESS_HINT, NO_FLAGS, &error);
    if (error != SCI_ERR_OK) return 1;

    SEOE(SCICreateMapSequence, remote_map, &sequence, NO_FLAGS);
    *remote_address = 1;

    clock_t start_time = clock();
    int response_count = 0;

    while (1) {
        if (((clock() - start_time) / CLOCKS_PER_SEC) >= 2) {
            // Write -1 to the remote counter after 2 seconds
            *remote_address = -1;
            printf("Timer triggered: wrote -1 to remote counter\n");
            printf("Response count: %d\n", response_count);
            break;
        }

        while (local_address->counter == curr_counter) {
            // busy wait
        }
        response_count++;
        curr_counter++;
        *remote_address = curr_counter+1;
    }

    SEOE(SCIRemoveSequence, sequence, NO_FLAGS);
    SEOE(SCIUnmapSegment, local_map, NO_FLAGS);
    SEOE(SCIUnmapSegment, remote_map, NO_FLAGS);
    SEOE(SCIDisconnectSegment, remote_segment, NO_FLAGS);
    SEOE(SCISetSegmentUnavailable, segment, ADAPTER_NO, NO_FLAGS);
    SEOE(SCIRemoveSegment, segment, NO_FLAGS);
    SCITerminate();
}