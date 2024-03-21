#ifndef KRITJOLIB_SISCI_COMMON
#define KRITJOLIB_SISCI_COMMON

#include "sisci_types.h"
#include "sisci_error.h"

void remote_connect_init(sci_desc_t v_dev, sci_remote_segment_t *remote_segment, unsigned int receiver_id);
void local_segment_init(sci_desc_t v_dev, sci_local_segment_t *local_segment, size_t size, void **local_address, sci_map_t *local_map, sci_cb_local_segment_t callback, void *callback_arg);

#endif //KRITJOLIB_SISCI_COMMON
