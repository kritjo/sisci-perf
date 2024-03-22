#ifndef KRITJOLIB_SISCI_DMA
#define KRITJOLIB_SISCI_DMA

#include <stdbool.h>

#include "sisci_types.h"
#include "sisci_arg_types.h"

void send_dma_segment(dma_args_t *args);
void dma_init(dma_args_t *args);
void dma_destroy(dma_args_t *args);
void dma_channel_destroy(dma_args_t *args, dma_channel_args_t *ch_args);
void dma_channel_init(dma_args_t *args, dma_channel_args_t *ch_args);

#endif //KRITJOLIB_SISCI_DMA
