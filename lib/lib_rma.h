#ifndef KRITJOLIB_SISCI_RMA
#define KRITJOLIB_SISCI_RMA

#include "sisci_types.h"

void rma_init(sci_remote_segment_t remote_segment, volatile void **remote_address, sci_map_t *remote_map);
void rma_sequence_init(sci_map_t *remote_map, sci_sequence_t *sequence);
void rma_flush(sci_sequence_t sequence);
void rma_check(sci_sequence_t sequence);
void rma_destroy_sequence(sci_sequence_t sequence);
void rma_destroy(sci_map_t remote_map);

#endif //KRITJOLIB_SISCI_RMA
