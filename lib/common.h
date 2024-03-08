#ifndef KRITJOLIB_SISCI_COMMON
#define KRITJOLIB_SISCI_COMMON

#include "sisci_types.h"
#include "sisci_error.h"

void remote_connect_init(sci_desc_t v_dev, sci_remote_segment_t *remote_segment, unsigned int receiver_id);

#endif //KRITJOLIB_SISCI_COMMON
