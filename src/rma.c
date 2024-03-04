#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "rma.h"

#include "sisci_api.h"
#include "sisci_types.h"

#include "rdma_buff.h"
#include "error_util.h"
#include "sisci_glob_defs.h"

void rma(sci_remote_segment_t remote_segment, bool check) {
    volatile rdma_buff_t *remote_address;
    sci_map_t remote_map;
    sci_error_t error;
    size_t remote_segment_size;
    sci_sequence_t sequence;
    sci_sequence_status_t sequence_status = SCI_SEQ_OK;
    rdma_buff_t rdma_buff;

    rdma_buff.done = 0;
    strcpy(rdma_buff.word, "OK");

    remote_segment_size = SCIGetRemoteSegmentSize(remote_segment);

    remote_address = (volatile rdma_buff_t*)SCIMapRemoteSegment(
            remote_segment, &remote_map,0 /* offset */, remote_segment_size,
            0 /* address hint */, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIMapRemoteSegment", true);

    SCICreateMapSequence(remote_map, &sequence, NO_FLAGS, &error);
    print_sisci_error(&error, "SCICreateMapSequence", true);

    if (check) {
        do {
            sequence_status = SCIStartSequence(sequence, NO_FLAGS, &error);
            print_sisci_error(&error, "SCIStartSequence", true);
        } while (sequence_status == SCI_SEQ_RETRIABLE || sequence_status == SCI_SEQ_PENDING);
        if (sequence_status == SCI_SEQ_NOT_RETRIABLE) {
            fprintf(stderr, "SCIStartSequence failed, fatal error\n");
            exit(EXIT_FAILURE);
        }
    }

    *remote_address = rdma_buff;
    remote_address->done = 1;
    printf("Wrote to remote segment\n");

    if (check) {
        printf("Flushing remote segment\n");
        SCIFlush(sequence, NO_FLAGS);

        printf("Checking sequence\n");
        sequence_status = SCICheckSequence(sequence, NO_FLAGS, &error);
        print_sisci_error(&error, "SCICheckSequence", true);
        if (sequence_status == SCI_SEQ_OK) printf("Sequence OK\n");
        else {
            fprintf(stderr, "Sequence not OK\n");
            exit(EXIT_FAILURE);
        }

        SCIRemoveSequence(sequence, NO_FLAGS, &error);
        print_sisci_error(&error, "SCIRemoveSequence", false);
    }

    SCIUnmapSegment(remote_map, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIUnmapSegment", false);
}