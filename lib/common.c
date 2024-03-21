#include <stdio.h>
#include <stdbool.h>

#include "common.h"
#include "sisci_api.h"
#include "sisci_glob_defs.h"
#include "error_util.h"

void remote_connect_init(sci_desc_t v_dev, sci_remote_segment_t *remote_segment, unsigned int receiver_id) {
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

void local_segment_init(sci_desc_t v_dev, sci_local_segment_t *local_segment, size_t size, void **local_address, sci_map_t *local_map, sci_cb_local_segment_t callback, void *callback_arg, unsigned int flags) {
    sci_error_t error;

    SCICreateSegment(v_dev,
                     local_segment,
                     RECEIVER_SEG_ID,
                     size,
                     callback,
                     callback_arg,
                     flags,
                     &error);
    print_sisci_error(&error, "SCICreateSegment", true);

    SCIPrepareSegment(*local_segment,
                      ADAPTER_NO,
                      NO_FLAGS,
                      &error);
    print_sisci_error(&error, "SCIPrepareSegment", true);

    *local_address = SCIMapLocalSegment(*local_segment,
                                                  local_map,
                                                  NO_OFFSET,
                                                  RECEIVER_SEG_SIZE,
                                                  NO_SUG_ADDR,
                                                  NO_FLAGS,
                                                  &error);
    print_sisci_error(&error, "SCIMapLocalSegment", true);
}