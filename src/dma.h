#ifndef KRITJO_SISCI_DMA
#define KRITJO_SISCI_DMA

#include <stdbool.h>

#include "sisci_types.h"

void dma_send_test(sci_desc_t v_dev, sci_remote_segment_t remote_segment, bool use_sysdma);

#endif //KRITJO_SISCI_DMA
