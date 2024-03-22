#include <stdbool.h>
#include <stdio.h>
#include "lib_rma.h"
#include "sisci_types.h"
#include "sisci_api.h"
#include "error_util.h"
#include "sisci_glob_defs.h"
#include "sisci_arg_types.h"

void rma_init(segment_remote_args_t *remote) {
    OUT_NON_NULL_WARNING(remote->segment_size);
    sci_error_t error;

    remote->segment_size = SCIGetRemoteSegmentSize(remote->segment);

    remote->address = SCIMapRemoteSegment(
            remote->segment, &remote->map,0 /* offset */, remote->segment_size,
            0 /* address hint */, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIMapRemoteSegment", true);
}

void rma_sequence_start(sci_sequence_t sequence) {
    sci_error_t error;
    sci_sequence_status_t sequence_status;

    do {
        sequence_status = SCIStartSequence(sequence, NO_FLAGS, &error);
        print_sisci_error(&error, "SCIStartSequence", true);
    } while (sequence_status == SCI_SEQ_RETRIABLE || sequence_status == SCI_SEQ_PENDING);
    switch (sequence_status) {
        case SCI_SEQ_OK:
            printf("Sequence OK\n");
            break;
        case SCI_SEQ_NOT_RETRIABLE:
            fprintf(stderr, "Sequence not retriable\n");
            break;
        default:
            fprintf(stderr, "Unknown sequence status\n");
    }
}

void rma_sequence_init(sci_map_t map, sci_sequence_t *sequence) {
    sci_error_t error;

    SCICreateMapSequence(map, sequence, NO_FLAGS, &error);
    print_sisci_error(&error, "SCICreateMapSequence", true);

    rma_sequence_start(*sequence);
}

void rma_flush(sci_sequence_t sequence) {
    printf("Flushing remote segment\n");
    SCIFlush(sequence, NO_FLAGS);
}

void rma_check(sci_sequence_t sequence) {
    sci_sequence_status_t sequence_status;
    sci_error_t error;

    printf("Checking sequence\n");
    do {
        sequence_status = SCICheckSequence(sequence, NO_FLAGS, &error);
        print_sisci_error(&error, "SCICheckSequence", true);
    } while (sequence_status == SCI_SEQ_RETRIABLE);
    switch (sequence_status) {
        case SCI_SEQ_OK:
            printf("Sequence OK\n");
            break;
        case SCI_SEQ_PENDING:
            fprintf(stderr, "Sequence pending\n");
            rma_sequence_start(sequence);
        case SCI_SEQ_NOT_RETRIABLE:
            fprintf(stderr, "Sequence not retriable, starting sequence\n");
            rma_sequence_start(sequence);
        default:
            fprintf(stderr, "Unknown sequence status\n");
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