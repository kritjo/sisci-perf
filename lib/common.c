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

void local_segment_init(sci_desc_t v_dev, segment_args_t *args, unsigned int create_flags) {
    sci_error_t error;

    SCICreateSegment(v_dev,
                     &args->local.segment,
                     RECEIVER_SEG_ID,
                     args->local.segment_size,
                     args->callback_args.callback,
                     args->callback_args.arg,
                     create_flags,
                     &error);
    print_sisci_error(&error, "SCICreateSegment", true);

    SCIPrepareSegment(args->local.segment,
                      ADAPTER_NO,
                      NO_FLAGS,
                      &error);
    print_sisci_error(&error, "SCIPrepareSegment", true);

    args->local.address = SCIMapLocalSegment(args->local.segment,
                                                  &args->local.map,
                                                  NO_OFFSET,
                                                  args->local.segment_size,
                                                  NO_SUG_ADDR,
                                                  NO_FLAGS,
                                                  &error);
    print_sisci_error(&error, "SCIMapLocalSegment", true);
}