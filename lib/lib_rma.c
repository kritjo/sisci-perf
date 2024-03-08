#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include "lib_rma.h"
#include "sisci_types.h"
#include "sisci_api.h"
#include "error_util.h"
#include "sisci_glob_defs.h"

void rma_init(sci_remote_segment_t remote_segment, volatile void **remote_address, sci_map_t *remote_map) {
    size_t remote_segment_size;
    sci_error_t error;

    remote_segment_size = SCIGetRemoteSegmentSize(remote_segment);

    *remote_address = SCIMapRemoteSegment(
            remote_segment, remote_map,0 /* offset */, remote_segment_size,
            0 /* address hint */, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIMapRemoteSegment", true);
}

void rma_sequence_init(sci_map_t *remote_map, sci_sequence_t *sequence) {
    sci_error_t error;
    sci_sequence_status_t sequence_status;

    SCICreateMapSequence(*remote_map, sequence, NO_FLAGS, &error);
    print_sisci_error(&error, "SCICreateMapSequence", true);

    do {
        sequence_status = SCIStartSequence(*sequence, NO_FLAGS, &error);
        print_sisci_error(&error, "SCIStartSequence", true);
    } while (sequence_status == SCI_SEQ_RETRIABLE || sequence_status == SCI_SEQ_PENDING);
    if (sequence_status == SCI_SEQ_NOT_RETRIABLE) {
        fprintf(stderr, "SCIStartSequence failed, fatal error\n");
        exit(EXIT_FAILURE);
    }
}

void rma_flush(sci_sequence_t sequence) {
    printf("Flushing remote segment\n");
    SCIFlush(sequence, NO_FLAGS);
}

void rma_check(sci_sequence_t sequence) {
    sci_sequence_status_t sequence_status;
    sci_error_t error;

    printf("Checking sequence\n");
    sequence_status = SCICheckSequence(sequence, NO_FLAGS, &error);
    print_sisci_error(&error, "SCICheckSequence", true);
    if (sequence_status == SCI_SEQ_OK) printf("Sequence OK\n");
    else {
        fprintf(stderr, "Sequence not OK\n");
        exit(EXIT_FAILURE);
    }
}

void rma_destroy_sequence(sci_sequence_t sequence) {
    sci_error_t error;
    SCIRemoveSequence(sequence, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIRemoveSequence", false);
}

void rma_destroy(sci_map_t remote_map) {
    sci_error_t error;
    SCIUnmapSegment(remote_map, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIUnmapSegment", false);
}