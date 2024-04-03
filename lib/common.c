#include <stdio.h>
#include <stdbool.h>

#include "common.h"
#include "sisci_api.h"
#include "sisci_glob_defs.h"
#include "error_util.h"

void init_remote_connect(sci_desc_t v_dev, sci_remote_segment_t *remote_segment, unsigned int receiver_id) {
    int remote_reachable;
    size_t remote_segment_size;
    sci_error_t error;

    remote_reachable = SCIProbeNode(v_dev, ADAPTER_NO, receiver_id, NO_FLAGS, &error);
    printf("Probe said that receiver node (%d) was", receiver_id);
    if (remote_reachable) printf(" reachable!\n");
    else printf(" NOT reachable!\n");

    SCIConnectSegment(v_dev,
                      remote_segment,
                      receiver_id,
                      RECEIVER_SEG_ID,
                      ADAPTER_NO,
                      NO_CALLBACK,
                      NO_ARG,
                      SCI_INFINITE_TIMEOUT,
                      NO_FLAGS,
                      &error);
    print_sisci_error(&error, "SCIConnectSegment", true);

    remote_segment_size = SCIGetRemoteSegmentSize(*remote_segment);
    printf("Connected to remote segment of size %ld\n", remote_segment_size);
}

void destroy_remote_connect(sci_remote_segment_t remote_segment) {
    sci_error_t error;

    SCIDisconnectSegment(remote_segment, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIDisconnectSegment", false);
}

void init_local_segment(
        sci_desc_t v_dev,
        segment_local_args_t *local,
        segment_callback_args_t *callback,
        unsigned int segment_id,
        bool dma_source_only,
        unsigned int create_flags,
        unsigned int map_flags) {
    OUT_NON_NULL_WARNING(local->segment);
    OUT_NON_NULL_WARNING(local->map);
    OUT_NON_NULL_WARNING(local->offset);

    sci_error_t error;

    void *suggest_addr = local->address;

    SCICreateSegment(v_dev,
                     &local->segment,
                     segment_id,
                     local->segment_size,
                     callback != NO_CALLBACK ? callback->function : NO_CALLBACK,
                     callback != NO_CALLBACK ? callback->arg : NO_ARG,
                     create_flags,
                     &error);
    print_sisci_error(&error, "SCICreateSegment", true);

    SCIPrepareSegment(local->segment,
                      ADAPTER_NO,
                      dma_source_only ? SCI_FLAG_DMA_SOURCE_ONLY : NO_FLAGS,
                      &error);
    print_sisci_error(&error, "SCIPrepareSegment", true);

    local->address = SCIMapLocalSegment(local->segment,
                                       &local->map,
                                       local->offset,
                                       local->segment_size,
                                       suggest_addr,
                                        map_flags,
                                       &error);
    print_sisci_error(&error, "SCIMapLocalSegment", true);
}

void destroy_local_segment(segment_local_args_t *local) {
    sci_error_t error;

    SCIUnmapSegment(local->map, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIUnmapSegment", false);

    SCIRemoveSegment(local->segment, NO_FLAGS, &error);
    print_sisci_error(&error, "SCIRemoveSegment", false);
}