#ifndef KRITJOLIB_SISCI_COMMON
#define KRITJOLIB_SISCI_COMMON

#include "sisci_types.h"
#include "sisci_error.h"
#include "sisci_arg_types.h"

void init_remote_connect(sci_desc_t v_dev, sci_remote_segment_t *remote_segment, unsigned int receiver_id);
void destroy_remote_connect(sci_remote_segment_t remote_segment);
void init_local_segment(
        sci_desc_t v_dev,
        segment_local_args_t *local,
        segment_callback_args_t *callback,
        unsigned int segment_id,
        bool dma_source_only,
        unsigned int create_flags,
        unsigned int map_flags);
void destroy_local_segment(segment_local_args_t *local);

#endif //KRITJOLIB_SISCI_COMMON
