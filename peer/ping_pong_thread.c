#include <sisci_api.h>
#include <stdlib.h>
#include <pthread.h>
#include "ping_pong_thread.h"
#include "sisci_glob_defs.h"
#include "protocol.h"

static void ping_pong_thread_cleanup(void *arg) {
    ping_pong_cleanup_arg_t *cleanup_arg = (ping_pong_cleanup_arg_t *) arg;

    SEOE(SCIRemoveSequence, cleanup_arg->sequence, NO_FLAGS);
    SEOE(SCIUnmapSegment, cleanup_arg->map, NO_FLAGS);
    SEOE(SCIUnmapSegment, cleanup_arg->remote_map, NO_FLAGS);
    SEOE(SCIDisconnectSegment, cleanup_arg->remote_segment, NO_FLAGS);
}

void *ping_pong_thread_start(void *arg) {
    ping_pong_thread_arg_t args = *(ping_pong_thread_arg_t *) arg;
    sci_local_segment_t segment = args.segment;
    sci_map_t map;
    peer_ping_pong_segment_t *buffer;
    sci_error_t error;
    sci_remote_segment_t remote_segment;
    sci_map_t remote_map;
    sci_sequence_t sequence;
    unsigned char *remote_buffer;
    unsigned char curr_counter = 0;

    buffer = (typeof(buffer)) SCIMapLocalSegment(segment, &map, NO_OFFSET, SCIGetLocalSegmentSize(segment),
                                                 NO_SUGGESTED_ADDRESS, NO_FLAGS, &error);
    if (error != SCI_ERR_OK) {
        fprintf(stderr, "Failed to map segment: %s\n", SCIGetErrorString(error));
        exit(EXIT_FAILURE);
    }

    while (!buffer->initiator_ready) {
        SEOE(SCICacheSync, map, buffer, sizeof(peer_ping_pong_segment_t),
             SCI_FLAG_CACHE_INVALIDATE | SCI_FLAG_CACHE_FLUSH);
    }

    SEOE(SCIConnectSegment, args.sd, &remote_segment, args.initiator_node_id, buffer->initiator_ping_pong_segment_id, ADAPTER_NO,
         NO_CALLBACK, NO_ARG, SCI_INFINITE_TIMEOUT, NO_FLAGS);

    remote_buffer = (typeof(remote_buffer)) SCIMapRemoteSegment(remote_segment, &remote_map, NO_OFFSET,
                                                                SCIGetRemoteSegmentSize(remote_segment),
                                                                NO_SUGGESTED_ADDRESS, SCI_FLAG_IO_MAP_IOSPACE, &error);
    if (error != SCI_ERR_OK) {
        fprintf(stderr, "Failed to map remote segment: %s\n", SCIGetErrorString(error));
        exit(EXIT_FAILURE);
    }

    SEOE(SCICreateMapSequence, remote_map, &sequence, NO_FLAGS);
    SEOE(SCIStartSequence, sequence, NO_FLAGS);

    ping_pong_cleanup_arg_t cleanup_arg = {
            .map = map,
            .remote_segment = remote_segment,
            .remote_map = remote_map,
            .sequence = sequence
    };

    pthread_cleanup_push(ping_pong_thread_cleanup, &cleanup_arg)

    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    while (1) {
        while (buffer->counter == curr_counter) {
            SEOE(SCICacheSync, map, buffer, sizeof(peer_ping_pong_segment_t),
                 SCI_FLAG_CACHE_INVALIDATE | SCI_FLAG_CACHE_FLUSH);
        }
        curr_counter++;
        *remote_buffer = curr_counter;
        SCIFlush(sequence, NO_FLAGS);
    }

    pthread_cleanup_pop(1);
}