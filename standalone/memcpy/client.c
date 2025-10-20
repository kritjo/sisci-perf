#include "common.h"

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
    sci_error_t error;
    void* local_address = malloc(SEGMENT_SIZE);
    if (local_address == NULL) return 1;

    sci_remote_segment_t remote_segment;
    sci_map_t remote_map;
    volatile int* remote_address;
    sci_sequence_t remote_sequence;
    timer_start_t timer_start;
    double totalTimeUs;
    double totalBytes;
    double MB_pr_second;
    double averageTransferTime;

    SEOE(SCIInitialize, NO_FLAGS);
    SEOE(SCIOpen, &sd, NO_FLAGS);

    SEOE(SCIConnectSegment, sd, &remote_segment, remote_node_id, SEGMENT_ID, ADAPTER_NO, NO_CALLBACK, NO_ARGS, SCI_INFINITE_TIMEOUT, NO_FLAGS);
    remote_address = (volatile int*) SCIMapRemoteSegment(remote_segment, &remote_map, NO_OFFSET, SEGMENT_SIZE, NO_ADDRESS_HINT, NO_FLAGS, &error);
    if (error != SCI_ERR_OK) return 1;
    SEOE(SCICreateMapSequence, remote_map, &remote_sequence, NO_FLAGS);

    StartTimer(&timer_start);

    for (int i = 0; i < ILOOPS; i++) {
        SEOE(SCIMemCpy, remote_sequence, local_address, remote_map, NO_OFFSET, size, NO_FLAGS);
    }

    totalTimeUs = StopTimer(timer_start);

    totalBytes            = (double)size * ILOOPS;
    averageTransferTime   = (totalTimeUs/(double)ILOOPS);
    MB_pr_second          = totalBytes/totalTimeUs;
    printf("%7llu            %6.2f us              %7.2f MBytes/s\n",
          (unsigned long long) size, (double)averageTransferTime,
          (double)MB_pr_second);
          
    SEOE(SCIRemoveSequence, remote_sequence, NO_FLAGS);
    SEOE(SCIUnmapSegment, remote_map, NO_FLAGS);
    SEOE(SCIDisconnectSegment, remote_segment, NO_FLAGS);
    SEOE(SCIClose, sd, NO_FLAGS);
    SCITerminate();
    return 0;
}