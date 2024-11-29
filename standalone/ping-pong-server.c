#include "ping-pong-common.h"

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
    int* local_address;
    int curr_counter = 0;

    sci_remote_segment_t remote_segment;
    sci_map_t remote_map;
    volatile ping_pong_segment_t* remote_address;
    ping_pong_segment_t local_writeback_buffer;

    SEOE(SCIInitialize, NO_FLAGS);
    SEOE(SCIOpen, &sd, NO_FLAGS);

    SEOE(SCICreateSegment, sd, &segment, SERVER_SEGMENT_ID, SEGMENT_SIZE, NO_CALLBACK, NO_ARGS, NO_FLAGS);
    SEOE(SCIPrepareSegment, segment, ADAPTER_NO, NO_FLAGS);
    SEOE(SCISetSegmentAvailable, segment, ADAPTER_NO, NO_FLAGS);

    /* map the local segment */
    local_address = (int*) SCIMapLocalSegment(segment, &local_map, NO_OFFSET, SEGMENT_SIZE, NO_ADDRESS_HINT, NO_FLAGS, &error);
    if (error != SCI_ERR_OK) return 1;

    for (int i = 0; i < WRITEBACK_SIZE; i++) {
        local_writeback_buffer.writeback_buffer[i] = i;
    }

    do {
        SCIConnectSegment(sd, &remote_segment, remote_node_id, CLIENT_SEGMENT_ID, ADAPTER_NO, NO_CALLBACK, NO_ARGS, SCI_INFINITE_TIMEOUT, NO_FLAGS, &error);
    } while (error == SCI_ERR_NO_SUCH_SEGMENT);

    remote_address = (volatile ping_pong_segment_t*) SCIMapRemoteSegment(remote_segment, &remote_map, NO_OFFSET, SEGMENT_SIZE, NO_ADDRESS_HINT, NO_FLAGS, &error);
    if (error != SCI_ERR_OK) return 1;

    SEOE(SCICreateMapSequence, remote_map, &sequence, NO_FLAGS);

    while (*local_address >= 0) {
        while (*local_address == curr_counter) {
            // busy wait
        }

        curr_counter++;
        local_writeback_buffer.counter = curr_counter;

        SEOE(SCIMemCpy, sequence, (void*) &local_writeback_buffer, remote_map, NO_OFFSET, sizeof(ping_pong_segment_t), NO_FLAGS);
    }

    SEOE(SCIRemoveSequence, sequence, NO_FLAGS);
    SEOE(SCIUnmapSegment, local_map, NO_FLAGS);
    SEOE(SCIUnmapSegment, remote_map, NO_FLAGS);
    SEOE(SCIDisconnectSegment, remote_segment, NO_FLAGS);
    SEOE(SCISetSegmentUnavailable, segment, ADAPTER_NO, NO_FLAGS);
    SEOE(SCIRemoveSegment, segment, NO_FLAGS);
    SCITerminate();
}