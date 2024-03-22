#ifndef KRITJOLIB_SISCI_COMMON
#define KRITJOLIB_SISCI_COMMON

#include "sisci_types.h"
#include "sisci_error.h"
#include "sisci_arg_types.h"

void remote_connect_init(sci_desc_t v_dev, sci_remote_segment_t *remote_segment, unsigned int receiver_id);
void local_segment_init(sci_desc_t v_dev, segment_args_t *args, unsigned int create_flags);

#endif //KRITJOLIB_SISCI_COMMON
